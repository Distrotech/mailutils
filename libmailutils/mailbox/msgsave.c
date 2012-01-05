/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#include <config.h>

#include <mailutils/types.h>
#include <mailutils/diag.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/mailbox.h>
#include <mailutils/message.h>
#include <mailutils/stream.h>

int
mu_message_save_to_mailbox (mu_message_t msg, const char *toname, int perms)
{
  int rc = 0;
  mu_mailbox_t to = 0;

  if ((rc = mu_mailbox_create_default (&to, toname)))
    {
      mu_debug (MU_DEBCAT_MESSAGE, MU_DEBUG_ERROR, 		 
		("mu_mailbox_create_default (%s) failed: %s\n", toname,
		 mu_strerror (rc)));
      goto end;
    }

  if ((rc = mu_mailbox_open (to,
			     MU_STREAM_WRITE | MU_STREAM_CREAT
			     | (perms & MU_STREAM_IMASK))))
    {
      mu_debug (MU_DEBCAT_MESSAGE, MU_DEBUG_ERROR, 		 
		("mu_mailbox_open (%s) failed: %s", toname,
		 mu_strerror (rc)));
      goto end;
    }

  if ((rc = mu_mailbox_append_message (to, msg)))
    {
      mu_debug (MU_DEBCAT_MESSAGE, MU_DEBUG_ERROR, 		 
		("mu_mailbox_append_message (%s) failed: %s", toname,
		 mu_strerror (rc)));
      goto end;
    }

end:

  if (!rc)
    {
      if ((rc = mu_mailbox_close (to)))
        mu_debug (MU_DEBCAT_MESSAGE, MU_DEBUG_ERROR, 		 
		  ("mu_mailbox_close (%s) failed: %s", toname,
		   mu_strerror (rc)));
    }
  else
    mu_mailbox_close (to);

  mu_mailbox_destroy (&to);

  return rc;
}
