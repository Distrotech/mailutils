/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "pop3d.h"

static int is_virtual = 0;

#ifdef HAVE_MYSQL
# include "../MySql/MySql.h"
#endif

#ifdef USE_VIRTUAL_DOMAINS

static struct passwd *
pop3d_virtual (const char *u)
{
  struct passwd *pw;
  FILE *pfile;
  int i = 0, len = strlen (u), delim = 0;

  for (i = 0; i < len && delim == 0; i++)
    if (u[i] == '!' || u[i] == ':' || u[i] == '@')
      delim = i;

  if (delim == 0)
    return NULL;

  chdir ("/etc/domains");
  pfile = fopen (&u[delim+1], "r");
  while (pfile != NULL && (pw = fgetpwent (pfile)) != NULL)
    {
      if (strlen (pw->pw_name) == delim && !strncmp (u, pw->pw_name, delim))
	{
	  is_virtual = 1;
	  return pw;
	}
    }
  
  return NULL;
}

#endif

#ifdef USE_LIBPAM
#define COPY_STRING(s) (s) ? strdup(s) : NULL

static char *_pwd;
static char *_user;
static int _perr = 0;

#define PAM_ERROR if (_perr || (pamerror != PAM_SUCCESS)) \
    goto pam_errlab;

static int
PAM_gnupop3d_conv (int num_msg, const struct pam_message **msg,
		   struct pam_response **resp, void *appdata_ptr)
{
  int replies = 0;
  struct pam_response *reply = NULL;

  reply = malloc (sizeof (*reply) * num_msg);
  if (!reply)
    return PAM_CONV_ERR;

  for (replies = 0; replies < num_msg; replies++)
    {
      switch (msg[replies]->msg_style)
	{
	case PAM_PROMPT_ECHO_ON:
	  reply[replies].resp_retcode = PAM_SUCCESS;
	  reply[replies].resp = COPY_STRING (_user);
	  /* PAM frees resp */
	  break;

	case PAM_PROMPT_ECHO_OFF:
	  reply[replies].resp_retcode = PAM_SUCCESS;
	  reply[replies].resp = COPY_STRING (_pwd);
	  /* PAM frees resp */
	  break;

	case PAM_TEXT_INFO:
	case PAM_ERROR_MSG:
	  reply[replies].resp_retcode = PAM_SUCCESS;
	  reply[replies].resp = NULL;
	  break;

	default:
	  free (reply);
	  _perr = 1;
	  return PAM_CONV_ERR;
	}
    }
  *resp = reply;
  return PAM_SUCCESS;
}

static struct pam_conv PAM_conversation = { &PAM_gnupop3d_conv, NULL };
#endif /* USE_LIBPAM */

/* Basic user authentication. This also takes the PASS command and verifies
   the user name and password. Calls setuid() upon successful verification,
   otherwise it will (likely) return ERR_BAD_LOGIN */

int
pop3d_user (const char *arg)
{
  char *buf, pass[POP_MAXCMDLEN], *tmp, *cmd;
  struct passwd *pw;
  int status;
  int lockit = 1;
  char *mailbox_name;

  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  fprintf (ofile, "+OK\r\n");
  fflush (ofile);

  buf = pop3d_readline (ifile);
  cmd = pop3d_cmd (buf);
  tmp = pop3d_args (buf);

  if (strlen (tmp) > POP_MAXCMDLEN)
    {
      free (cmd);
      free (tmp);
      return ERR_TOO_LONG;
    }
  else
    {
      strncpy (pass, tmp, POP_MAXCMDLEN);
      /* strncpy () is lame, make sure the string is null terminated.  */
      pass[POP_MAXCMDLEN - 1] = '\0';
      free (tmp);
    }

  if (strlen (cmd) > 4)
    {
      free (cmd);
      return ERR_BAD_CMD;
    }

  if ((strcasecmp (cmd, "PASS") == 0))
    {
      free (cmd);

#ifdef _USE_APOP
      /* Check to see if they have an APOP password. If so, refuse USER/PASS */
      tmp = pop3d_apopuser (arg);
      if (tmp != NULL)
	{
	  syslog (LOG_INFO, "APOP user %s tried to log in with USER", arg);
	  free (tmp);
	  return ERR_BAD_LOGIN;
	}
#endif

      pw = mu_getpwnam (arg);

#ifdef USE_VIRTUAL_DOMAINS
      if (pw == NULL)
	pw = pop3d_virtual (arg);
#endif

      if (pw == NULL)
	{
	  syslog (LOG_INFO, "User '%s': nonexistent", arg);
	  return ERR_BAD_LOGIN;
	}

#ifndef USE_LIBPAM
      if (pw->pw_uid < 1)
	return ERR_BAD_LOGIN;
      if (strcmp (pw->pw_passwd, (char *) crypt (pass, pw->pw_passwd)))
	{
#ifdef HAVE_SHADOW_H
	  struct spwd *spw;
	  spw = getspnam ((char *) arg);
#ifdef HAVE_MYSQL
	  if (spw == NULL)
	    spw = getMspnam (arg);
#endif /* HAVE_MYSQL */
	  if (spw == NULL || strcmp (spw->sp_pwdp,
				     (char *) crypt (pass, spw->sp_pwdp)))
#endif /* HAVE_SHADOW_H */
	    {
	      syslog (LOG_INFO, "User '%s': authentication failed", arg);
	      return ERR_BAD_LOGIN;
	    }
	}
#else /* !USE_LIBPAM */
      {
	pam_handle_t *pamh;
	int pamerror;
	_user = (char *) arg;
	_pwd = pass;
	/* libpam doesn't log to LOG_MAIL */
	closelog ();
	pamerror = pam_start ("gnu-pop3d", arg, &PAM_conversation, &pamh);
	PAM_ERROR;
	pamerror = pam_authenticate (pamh, 0);
	PAM_ERROR;
	pamerror = pam_acct_mgmt (pamh, 0);
	PAM_ERROR;
	pamerror = pam_setcred (pamh, PAM_ESTABLISH_CRED);
	PAM_ERROR;
      pam_errlab:
	pam_end (pamh, PAM_SUCCESS);
	openlog ("gnu-pop3d", LOG_PID, LOG_FACILITY);
	if (pamerror != PAM_SUCCESS)
	  {
	    syslog (LOG_INFO, "User '%s': authentication failed", _user);
	    return ERR_BAD_LOGIN;
	  }
      }
#endif /* USE_LIBPAM */

      if (pw->pw_uid > 0 && !is_virtual)
	{
	  setuid (pw->pw_uid);
	  
	  mailbox_name = calloc (strlen (_PATH_MAILDIR) + 1
				 + strlen (pw->pw_name) + 1, 1);
	  sprintf (mailbox_name, "%s/%s", _PATH_MAILDIR, pw->pw_name);
	}
      else if (is_virtual)
	{
	  mailbox_name = calloc (strlen (pw->pw_dir) + strlen ("/INBOX"), 1);
	  sprintf (mailbox_name, "%s/INBOX", pw->pw_dir);
	}

      if ((status = mailbox_create (&mbox, mailbox_name)) != 0
	  || (status = mailbox_open (mbox, MU_STREAM_RDWR)) != 0)
	{
	  mailbox_destroy (&mbox);
	  /* For non existent mailbox, we fake.  */
	  if (status == ENOENT)
	    {
	      if (mailbox_create (&mbox, "/dev/null") != 0
		  || mailbox_open (mbox, MU_STREAM_READ) != 0)
		{
		  state = AUTHORIZATION;
		  free (mailbox_name);
		  return ERR_UNKNOWN;
		}
	    }
	  else
	    {
	      state = AUTHORIZATION;
	      free (mailbox_name);
	      return ERR_MBOX_LOCK;
	    }
	  lockit = 0;		/* Do not attempt to lock /dev/null ! */
	}
      free (mailbox_name);

      if (lockit && pop3d_lock ())
	{
	  mailbox_close (mbox);
	  mailbox_destroy (&mbox);
	  state = AUTHORIZATION;
	  return ERR_MBOX_LOCK;
	}

      username = strdup (pw->pw_name);
      if (username == NULL)
	pop3d_abquit (ERR_NO_MEM);
      state = TRANSACTION;

      fprintf (ofile, "+OK opened mailbox for %s\r\n", username);

      /* mailbox name */
      {
	url_t url = NULL;
	size_t total = 0;
	mailbox_get_url (mbox, &url);
	mailbox_messages_count (mbox, &total);
	syslog (LOG_INFO, "User '%s' logged in with mailbox '%s' (%d msgs)",
		username, url_to_string (url), total);
      }
      return OK;
    }
  else if (strcasecmp (cmd, "QUIT") == 0)
    {
      syslog (LOG_INFO, "Possible probe of account '%s'", arg);
      free (cmd);
      return pop3d_quit (pass);
    }

  free (cmd);
  return ERR_BAD_LOGIN;
}
