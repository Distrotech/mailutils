/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

int
imap4d_bye (int reason)
{
  return imap4d_bye0 (reason, NULL);
}

int
imap4d_bye0 (int reason, struct imap4d_command *command)
{
  struct passwd *pw = getpwuid (getuid ());
  const char *username;
  int status = EXIT_FAILURE;
  username = (pw) ? pw->pw_name : "Unknown";

  if (mbox)
    {
      mailbox_save_attributes (mbox);
      mailbox_close (mbox);
      mailbox_destroy (&mbox);
    }

  switch (reason)
    {
    case ERR_NO_MEM:
      util_out (RESP_BYE, "Server terminating no more resources.");
      syslog (LOG_ERR, "Out of memory");
      break;

    case ERR_SIGNAL:
      if (ofile) 
          util_out (RESP_BYE, "Quitting on signal");
      syslog (LOG_ERR, "Quitting on signal");
      break;

    case ERR_TIMEOUT:
      util_out (RESP_BYE, "Session timed out");
      if (state == STATE_NONAUTH)
        syslog (LOG_INFO, "Session timed out for no user");
      else
	syslog (LOG_INFO, "Session timed out for user: %s", username);
      break;

    case ERR_NO_OFILE:
      syslog (LOG_INFO, "No socket to send to");
      break;

    case OK:
      util_out (RESP_BYE, "Session terminating.");
      if (state == STATE_NONAUTH)
	syslog (LOG_INFO, "Session terminating");
      else
	syslog (LOG_INFO, "Session terminating for user: %s", username);
      status = EXIT_SUCCESS;
      break;

    default:
      util_out (RESP_BYE, "Quitting (reason unknown)");
      syslog (LOG_ERR, "Unknown quit");
      break;
    }

  if (status == EXIT_SUCCESS && command)
     util_finish (command, RESP_OK, "Completed");
  closelog ();
  exit (status);
}


