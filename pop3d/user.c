/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#ifdef USE_LIBPAM
#define COPY_STRING(s) (s) ? strdup(s) : NULL

static char *_pwd;
static char *_user;
static int _perr = 0;

#define PAM_ERROR if (_perr || (pamerror != PAM_SUCCESS)) { \
    pam_end(pamh, 0); \
    return ERR_BAD_LOGIN; }

static int
PAM_gnupop3d_conv (int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
{
  int replies = 0;
  struct pam_response *reply = NULL;

  reply = (struct pam_response *) malloc (sizeof (struct pam_response) * num_msg);
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

static struct pam_conv PAM_conversation =
{&PAM_gnupop3d_conv, NULL};
#endif /* USE_LIBPAM */

/* Basic user authentication. This also takes the PASS command and verifies
   the user name and password. Calls setuid() upon successful verification,
   otherwise it will (likely) return ERR_BAD_LOGIN */

int
pop3_user (const char *arg)
{
  char *buf, pass[POP_MAXCMDLEN], *tmp, *cmd;
  struct passwd *pw;
#ifdef USE_LIBPAM
  pam_handle_t *pamh;
  int pamerror;
#endif /* !USE_LIBPAM */

  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  fprintf (ofile, "+OK\r\n");
  fflush (ofile);

  buf = pop3_readline (ifile);
  cmd = pop3_cmd (buf);
  tmp = pop3_args (buf);
  free (buf);

  if (strlen (tmp) > POP_MAXCMDLEN)
    {
      free (cmd);
      free (tmp);
      return ERR_TOO_LONG;
    }
  else
    {
      strncpy (pass, tmp, POP_MAXCMDLEN);
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
      tmp = pop3_apopuser (arg);
      if (tmp != NULL)
	{
	  syslog (LOG_INFO, "APOP user %s tried to log in with USER", arg);
	  free (tmp);
	  return ERR_BAD_LOGIN;
	}
      free (tmp);
#endif

      pw = getpwnam (arg);
#ifndef USE_LIBPAM
      if (pw == NULL)
	return ERR_BAD_LOGIN;
      if (pw->pw_uid < 1)
	return ERR_BAD_LOGIN;
      if (strcmp (pw->pw_passwd, crypt (pass, pw->pw_passwd)))
	{
#ifdef HAVE_SHADOW_H
	  struct spwd *spw;
	  spw = getspnam (arg);
	  if (spw == NULL)
	    return ERR_BAD_LOGIN;
	  if (strcmp (spw->sp_pwdp, crypt (pass, spw->sp_pwdp)))
#endif /* HAVE_SHADOW_H */
	    return ERR_BAD_LOGIN;
	}
#else /* !USE_LIBPAM */
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
      pam_end (pamh, PAM_SUCCESS);
      openlog ("gnu-pop3d", LOG_PID, LOG_MAIL);
#endif /* USE_LIBPAM */

#ifdef MAILSPOOLHOME
      if (pw != NULL)
	{
	  chdir (pw->pw_dir);
	  mbox = mbox_open (MAILSPOOLHOME);
	}
      else
	mbox = NULL;

      if (mbox == NULL)		/* We should check errno here... */
	{
	  chdir (_PATH_MAILDIR);
#endif /* MAILSPOOLHOME */
	  mbox = mbox_open (arg);
	  if (mbox == NULL)	/* And here */
	    {
		state = AUTHORIZATION;
		return ERR_MBOX_LOCK;
	    }
#ifdef MAILSPOOLHOME
	}
#endif
      username = strdup (arg);
      if (username == NULL)
	pop3_abquit (ERR_NO_MEM);
      state = TRANSACTION;

      if (pw != NULL && pw->pw_uid > 1)
	setuid (pw->pw_uid);

      mbox_lock (mbox, MO_RLOCK | MO_WLOCK);
      
      fprintf (ofile, "+OK opened mailbox for %s\r\n", username);
      syslog (LOG_INFO, "User '%s' logged in with mailbox '%s'", username,
	      mbox->name);
      return OK;
    }
  else if (strcasecmp (cmd, "QUIT") == 0)
    {
      free (cmd);
      return pop3_quit (pass);
    }

  free (cmd);
  return ERR_BAD_LOGIN;
}
