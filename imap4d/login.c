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

#include "imap4d.h"

#ifdef HAVE_MYSQL
#include "../MySql/MySql.h"
#endif

#ifdef USE_LIBPAM
#define COPY_STRING(s) (s) ? strdup(s) : NULL

static char *_pwd;
static char *_user;
static int _perr = 0;

#define PAM_ERROR if (_perr || (pamerror != PAM_SUCCESS)) { \
    pam_end(pamh, 0); \
    return util_finish (command, RESP_NO, "User name or passwd rejected"); }

static int
PAM_gnuimap4d_conv (int num_msg, const struct pam_message **msg,
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

static struct pam_conv PAM_conversation = { &PAM_gnuimap4d_conv, NULL };
#endif /* USE_LIBPAM */

int
imap4d_login (struct imap4d_command *command, char *arg)
{
  struct passwd *pw;
  char *sp = NULL, *username, *pass;
#ifdef USE_LIBPAM
  pam_handle_t *pamh;
  int pamerror;
#endif /* !USE_LIBPAM */

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");
  username = util_getword (arg, &sp);
  pass = util_getword (NULL, &sp);

  /* Remove the double quotes.  */
  util_unquote (&username);
  util_unquote (&pass);

  if (username == NULL || *username == '\0' || pass == NULL)
    return util_finish (command, RESP_NO, "Too few args");
  else if (util_getword (NULL, &sp))
    return util_finish (command, RESP_NO, "Too many args");

  pw = mu_getpwnam (username);
  if (pw == NULL)
    return util_finish (command, RESP_NO, "User name or passwd rejected");

#ifndef USE_LIBPAM
  if (pw->pw_uid < 1)
    return util_finish (command, RESP_NO, "User name or passwd rejected");
  if (strcmp (pw->pw_passwd, (char *)crypt (pass, pw->pw_passwd)))
    {
#ifdef HAVE_SHADOW_H
      struct spwd *spw;
      spw = getspnam (username);
      if (spw == NULL || strcmp (spw->sp_pwdp, (char *)crypt (pass, spw->sp_pwdp)))
#ifdef HAVE_MYSQL
      {
         spw = getMspnam (username);
         if (spw == NULL || strcmp (spw->sp_pwdp, (char *)crypt (pass, spw->sp_pwdp)))
           return util_finish (command, RESP_NO, "User name or passwd rejected");
      }
#else /* HAVE_MYSQL */
#endif /* HAVE_SHADOW_H */
	return util_finish (command, RESP_NO, "User name or passwd rejected");
#endif /* HAVE_MYSQL */
    }

#else /* !USE_LIBPAM */
      _user = (char *) username;
      _pwd = pass;
      /* libpam doesn't log to LOG_MAIL */
      closelog ();
      pamerror = pam_start (pam_service, username, &PAM_conversation, &pamh);
      PAM_ERROR;
      pamerror = pam_authenticate (pamh, 0);
      PAM_ERROR;
      pamerror = pam_acct_mgmt (pamh, 0);
      PAM_ERROR;
      pamerror = pam_setcred (pamh, PAM_ESTABLISH_CRED);
      PAM_ERROR;
      pam_end (pamh, PAM_SUCCESS);
      openlog ("gnu-imap4d", LOG_PID, log_facility);
#endif /* USE_LIBPAM */

  if (pw->pw_uid > 0 && !mu_virtual_domain)
    setuid (pw->pw_uid);

  homedir = mu_normalize_path (strdup (pw->pw_dir), "/");
  /* FIXME: Check for errors.  */
  chdir (homedir);
  namespace_init(pw->pw_dir);
  syslog (LOG_INFO, "User '%s' logged in", username);
  return util_finish (command, RESP_OK, "Completed");
}
