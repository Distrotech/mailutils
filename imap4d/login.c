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

/*
 * FIXME: this should support PAM, shadow, and normal password
 */
#ifdef USE_LIBPAM
#define COPY_STRING(s) (s) ? strdup(s) : NULL

static char *_pwd;
static char *_user;
static int _perr = 0;

#define PAM_ERROR if (_perr || (pamerror != PAM_SUCCESS)) { \
    pam_end(pamh, 0); \
    return util_finish (command, RESP_NO, "User name or passwd rejected"); }

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

  if (username == NULL || pass == NULL)
    return util_finish (command, RESP_NO, "Too few args");
  else if (util_getword (NULL, &sp))
    return util_finish (command, RESP_NO, "Too many args");

  pw = getpwnam (arg);
#ifndef USE_LIBPAM
  if (pw == NULL || pw->pw_uid < 1)
    return util_finish (command, RESP_NO, "User name or passwd rejected");
  if (strcmp (pw->pw_passwd, crypt (pass, pw->pw_passwd)))
    {
#ifdef HAVE_SHADOW_H
      struct spwd *spw;
      spw = getspnam (arg);
      if (spw == NULL || strcmp (spw->sp_pwdp, crypt (pass, spw->sp_pwdp)))
#endif /* HAVE_SHADOW_H */
	return util_finish (command, RESP_NO, "User name or passwd rejected");
    }
#else /* !USE_LIBPAM */
      _user = (char *) arg;
      _pwd = pass;
      /* libpam doesn't log to LOG_MAIL */
      closelog ();
      pamerror = pam_start ("gnu-imap4d", arg, &PAM_conversation, &pamh);
      PAM_ERROR;
      pamerror = pam_authenticate (pamh, 0);
      PAM_ERROR;
      pamerror = pam_acct_mgmt (pamh, 0);
      PAM_ERROR;
      pamerror = pam_setcred (pamh, PAM_ESTABLISH_CRED);
      PAM_ERROR;
      pam_end (pamh, PAM_SUCCESS);
      openlog ("gnu-imap4d", LOG_PID, LOG_MAIL);
#endif /* USE_LIBPAM */

  if (pw->pw_uid > 1)
    setuid (pw->pw_uid);
  return util_finish (command, RESP_OK, "Completed");
}
