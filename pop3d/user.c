/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "pop3d.h"

int
pop3d_user (const char *arg)
{
  char *buf, pass[POP_MAXCMDLEN], *tmp, *cmd;
  char buffer[512];
  int status;
  int lockit = 1;
  struct mu_auth_data *auth_data;
  
  if (state != AUTHORIZATION)
    return ERR_WRONG_STATE;

  if ((strlen (arg) == 0) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  pop3d_outf ("+OK\r\n");
  pop3d_flush_output ();

  buf = pop3d_readline (buffer, sizeof (buffer));
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

  if (strcasecmp (cmd, "PASS") == 0)
    {
      int rc;

      free (cmd);

#ifdef _USE_APOP
      /* Check to see if they have an APOP password. If so, refuse USER/PASS */
      tmp = pop3d_apopuser (arg);
      if (tmp != NULL)
	{
	  syslog (LOG_INFO, _("APOP user %s tried to log in with USER"), arg);
	  free (tmp);
	  return ERR_BAD_LOGIN;
	}
#endif

      auth_data = mu_get_auth_by_name (arg);

      if (auth_data == NULL)
	{
	  syslog (LOG_INFO, _("User `%s': nonexistent"), arg);
	  return ERR_BAD_LOGIN;
	}

      rc = mu_authenticate (auth_data, pass);
      openlog ("gnu-pop3d", LOG_PID, log_facility);

      if (rc)
	{
	  syslog (LOG_INFO, _("User `%s': authentication failed"), arg);
	  mu_auth_data_free (auth_data);
	  return ERR_BAD_LOGIN;
	}
    }
  else if (strcasecmp (cmd, "QUIT") == 0)
    {
      syslog (LOG_INFO, _("Possible probe of account `%s'"), arg);
      free (cmd);
      return pop3d_quit (pass);
    }
  else
    {
      free (cmd);
      return ERR_BAD_CMD;
    }

  if (check_login_delay (auth_data->name))
    {
      syslog (LOG_INFO,
	      _("User `%s' tried to log in within the minimum allowed delay"),
	      auth_data->name);
      state = AUTHORIZATION;
      mu_auth_data_free (auth_data);
      return ERR_LOGIN_DELAY;
    }
  
  if (auth_data->change_uid)
    setuid (auth_data->uid);
  
  if ((status = mu_mailbox_create (&mbox, auth_data->mailbox)) != 0
      || (status = mu_mailbox_open (mbox, MU_STREAM_RDWR)) != 0)
    {
      mu_mailbox_destroy (&mbox);
      /* For non existent mailbox, we fake.  */
      if (status == ENOENT)
	{
	  if (mu_mailbox_create (&mbox, "/dev/null") != 0
	      || mu_mailbox_open (mbox, MU_STREAM_READ) != 0)
	    {
	      state = AUTHORIZATION;
	      mu_auth_data_free (auth_data);
	      return ERR_UNKNOWN;
	    }
	}
      else
	{
	  state = AUTHORIZATION;
	  mu_auth_data_free (auth_data);
	  return ERR_MBOX_LOCK;
	}
      lockit = 0;		/* Do not attempt to lock /dev/null ! */
    }
  
  if (lockit && pop3d_lock ())
    {
      mu_mailbox_close (mbox);
      mu_mailbox_destroy (&mbox); 
      mu_auth_data_free (auth_data);
      state = AUTHORIZATION;
      return ERR_MBOX_LOCK;
    }
  
  username = strdup (auth_data->name);
  if (username == NULL)
    pop3d_abquit (ERR_NO_MEM);
  state = TRANSACTION;

  mu_auth_data_free (auth_data);

  pop3d_outf ("+OK opened mailbox for %s\r\n", username);

  if (undelete_on_startup)
    pop3d_undelete_all ();
  
  /* mailbox name */
  {
    mu_url_t url = NULL;
    size_t total = 0;
    mu_mailbox_get_url (mbox, &url);
    mu_mailbox_messages_count (mbox, &total);
    syslog (LOG_INFO,
	    ngettext ("User `%s' logged in with mailbox `%s' (%d message)",
		      "User `%s' logged in with mailbox `%s' (%d messages)",
		      total),
	    username, mu_url_to_string (url), total);
  }
  return OK;
}

