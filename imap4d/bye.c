/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 
   2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"

int
imap4d_bye (int reason)
{
  return imap4d_bye0 (reason, NULL);
}

int
imap4d_bye0 (int reason, struct imap4d_command *command)
{
  int status = EXIT_FAILURE;

  if (mbox)
    {
      mu_mailbox_flush (mbox, 0);
      mu_mailbox_close (mbox);
      mu_mailbox_destroy (&mbox);
    }

  switch (reason)
    {
    case ERR_NO_MEM:
      util_out (RESP_BYE, "Server terminating no more resources.");
      syslog (LOG_ERR, _("Out of memory"));
      break;

    case ERR_SIGNAL:
      syslog (LOG_ERR, _("Quitting on signal"));
      exit (status);

    case ERR_TIMEOUT:
      util_out (RESP_BYE, "Session timed out");
      if (state == STATE_NONAUTH)
        syslog (LOG_INFO, _("Session timed out for no user"));
      else
	syslog (LOG_INFO, _("Session timed out for user: %s"), auth_data->name);
      break;

    case ERR_NO_OFILE:
      syslog (LOG_INFO, _("No socket to send to"));
      break;

    case ERR_MAILBOX_CORRUPTED:
      syslog (LOG_ERR, _("Mailbox modified by third party"));
      break;
      
    case OK:
      util_out (RESP_BYE, "Session terminating.");
      if (state == STATE_NONAUTH)
	syslog (LOG_INFO, _("Session terminating"));
      else
	syslog (LOG_INFO, _("Session terminating for user: %s"), auth_data->name);
      status = EXIT_SUCCESS;
      break;

    default:
      util_out (RESP_BYE, "Quitting (reason unknown)");
      syslog (LOG_ERR, _("Quitting (numeric reason %d)"), reason);
      break;
    }

  if (status == EXIT_SUCCESS && command)
     util_finish (command, RESP_OK, "Completed");

  util_bye ();

  closelog ();
  exit (status);
}

