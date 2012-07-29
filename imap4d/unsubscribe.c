/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2007-2012 Free Software Foundation, Inc.

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

#include "imap4d.h"
#include <mailutils/property.h>

/*
6.3.7.  UNSUBSCRIBE Command

   Arguments:  mailbox name

   Responses:  no specific responses for this command

   Result:     OK - unsubscribe completed
               NO - unsubscribe failure: can't unsubscribe that name
               BAD - command unknown or arguments invalid

      The UNSUBSCRIBE command removes the specified mailbox name from
      the server's set of "active" or "subscribed" mailboxes as returned
      by the LSUB command.  This command returns a tagged OK response
      only if the unsubscription is successful.
*/
int
imap4d_unsubscribe (struct imap4d_session *session,
                    struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *name;
  mu_property_t prop;

  if (imap4d_tokbuf_argc (tok) != 3)
    return io_completion_response (command, RESP_BAD, "Invalid arguments");

  name = imap4d_tokbuf_getarg (tok, IMAP4_ARG_1);

  prop = open_subscription ();
  if (!prop)
    return io_completion_response (command, RESP_NO, "Cannot unsubscribe");
  rc = mu_property_unset (prop, name);
  if (rc == MU_ERR_NOENT)
    rc = 0;
  else if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_property_unset", name, rc);
  else
    {
      rc = mu_property_save (prop);
      if (rc)
	mu_diag_funcall (MU_DIAG_ERROR, "mu_property_save", NULL, rc);
    }  
  mu_property_destroy (&prop);
  
  if (rc)
    return io_completion_response (command, RESP_NO, "Cannot unsubscribe");
   
  return io_completion_response (command, RESP_OK, "Completed");
}
