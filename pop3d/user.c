/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2003, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "pop3d.h"

struct mu_auth_data *auth_data;

int
pop3d_begin_session ()
{
  mu_url_t url = NULL;
  size_t total = 0;

  mu_diag_output (MU_DIAG_INFO, _("POP3 login: user `%s', source %s"),
		  auth_data->name, auth_data->source);
  
  if (check_login_delay (auth_data->name))
    {
      mu_diag_output (MU_DIAG_INFO,
	      _("user `%s' tried to log in within the minimum allowed delay"),
	      auth_data->name);
      state = AUTHORIZATION;
      mu_auth_data_destroy (&auth_data);
      return ERR_LOGIN_DELAY;
    }
  
  if (auth_data->change_uid)
    setuid (auth_data->uid);
  
  if (manlock_open_mailbox (&mbox, auth_data->mailbox, 0,
			    MU_STREAM_CREAT | MU_STREAM_RDWR))
    {
      mu_auth_data_destroy (&auth_data);
      state = AUTHORIZATION;
      return ERR_MBOX_LOCK;
    }
  
  username = mu_strdup (auth_data->name);
  state = TRANSACTION;

  pop3d_outf ("+OK opened mailbox for %s\n", username);

  if (undelete_on_startup)
    pop3d_undelete_all ();

  deliver_pending_bulletins ();
  
  /* log mailbox stats */
  mu_mailbox_get_url (mbox, &url);
  mu_mailbox_messages_count (mbox, &total);
  mu_diag_output (MU_DIAG_INFO,
	    ngettext ("user `%s' logged in with mailbox `%s' (%s message)",
		      "user `%s' logged in with mailbox `%s' (%s messages)",
		      (unsigned long) total),
	    username, mu_url_to_string (url), mu_umaxtostr (0, total));

  return OK;
}

int
pop3d_user (char *arg, struct pop3d_session *sess)
{
  char *buf, *pass, *cmd;
  char buffer[512];
  int xscript_level;
  
  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  pop3d_outf ("+OK\n");
  pop3d_flush_output ();

  xscript_level = set_xscript_level (MU_XSCRIPT_SECURE);
  buf = pop3d_readline (buffer, sizeof (buffer));
  pop3d_parse_command (buf, &cmd, &pass);
  set_xscript_level (xscript_level);
  
  if (mu_c_strcasecmp (cmd, "PASS") == 0)
    {
      int rc;

#ifdef _USE_APOP
      /* Check to see if they have an APOP password. If so, refuse USER/PASS */
      tmp = pop3d_apopuser (arg);
      if (tmp != NULL)
	{
	  mu_diag_output (MU_DIAG_INFO,
			  _("APOP user %s tried to log in with USER"), arg);
	  return ERR_BAD_LOGIN;
	}
#endif

      auth_data = mu_get_auth_by_name (arg);

      if (auth_data == NULL)
	{
	  mu_diag_output (MU_DIAG_INFO, _("user `%s' nonexistent"), arg);
	  return ERR_BAD_LOGIN;
	}

      rc = mu_authenticate (auth_data, pass);
      openlog (MU_LOG_TAG (), LOG_PID, mu_log_facility);

      if (rc)
	{
	  mu_diag_output (MU_DIAG_INFO,
			  _("user `%s': authentication failed"), arg);
	  mu_auth_data_destroy (&auth_data);
	  return ERR_BAD_LOGIN;
	}
    }
  else if (mu_c_strcasecmp (cmd, "QUIT") == 0)
    {
      mu_diag_output (MU_DIAG_INFO, _("possible probe of account `%s'"), arg);
      return pop3d_quit (pass, sess);
    }
  else
    {
      return ERR_BAD_CMD;
    }

  return pop3d_begin_session ();
}

