/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2006,
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
imap4d_login (struct imap4d_command *command, char *arg)
{
  char *sp = NULL, *username, *pass;
  int rc;

  if (login_disabled || tls_required)    
    return util_finish (command, RESP_NO, "Command disabled");

  username = util_getword (arg, &sp);
  pass = util_getword (NULL, &sp);

  /* Remove the double quotes.  */
  util_unquote (&username);
  util_unquote (&pass);

  if (username == NULL || *username == '\0' || pass == NULL)
    return util_finish (command, RESP_NO, "Too few args");
  else if (util_getword (NULL, &sp))
    return util_finish (command, RESP_NO, "Too many args");

  auth_data = mu_get_auth_by_name (username);

  if (auth_data == NULL)
    {
      mu_diag_output (MU_DIAG_INFO, _("User `%s': nonexistent"), username);
      return util_finish (command, RESP_NO, "User name or passwd rejected");
    }

  rc = mu_authenticate (auth_data, pass);
  openlog ("gnu-imap4d", LOG_PID, log_facility);
  if (rc)
    {
      mu_diag_output (MU_DIAG_INFO, _("Login failed: %s"), username);
      return util_finish (command, RESP_NO, "User name or passwd rejected");
    }

  if (imap4d_session_setup0 ())
    return util_finish (command, RESP_NO, "User name or passwd rejected");
  return util_finish (command, RESP_OK, "Completed");
}

