/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "pop3d.h"

/* Resets the connection so that no messages are marked as deleted */

int
pop3d_rset (const char *arg)
{
  size_t i;
  size_t total = 0;

  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  mailbox_messages_count (mbox, &total);

  for (i = 1; i <= total; i++)
    {
      message_t msg = NULL;
      attribute_t attr = NULL;
      mailbox_get_message (mbox, i, &msg);
      message_get_attribute (msg, &attr);
      if (attribute_is_userflag (attr, POP3_ATTRIBUTE_DELE))
	attribute_unset_userflag (attr, POP3_ATTRIBUTE_DELE);
    }
  pop3d_outf ("+OK\r\n");
  return OK;
}
