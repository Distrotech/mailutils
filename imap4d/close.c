/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "imap4d.h"

/*
 */

int
imap4d_close (struct imap4d_command *command, char *arg)
{
  (void)arg;
  /* FIXME: Check and report errors.  */
  /* The CLOSE command permanently removes from the currently selected
     mailbox all messages that have the \\Deleted flag set, and returns
     to authenticated state from selected state.  */
  mailbox_flush (mbox, 1);
  /* No messages are removed, and no error is give, if the mailbox is
     selected by an EXAMINE command or is otherwise selected read-only.  */
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return util_finish (command, RESP_OK, "Completed");
}
