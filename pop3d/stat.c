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

/* Prints the number of messages and the total size of all messages */

int
pop3d_stat (const char *arg)
{
  size_t mesgno;
  size_t size = 0;
  size_t lines = 0;
  size_t total = 0;
  size_t num = 0;
  size_t tsize = 0;
  message_t msg = NULL;
  attribute_t attr = NULL;

  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  /* rfc1939: if the POP3 server host internally represents end-of-line as a
     single character, then the POP3 server simply counts each occurrence of
     this character in a message as two octets. */
  mailbox_messages_count (mbox, &total);
  for (mesgno = 1; mesgno <= total; mesgno++)
    {
      mailbox_get_message (mbox, mesgno, &msg);
      message_get_attribute (msg, &attr);
      /* rfc1939: Note that messages marked as deleted are not counted in
	 either total.  */
      if (!pop3d_is_deleted (attr))
	{
	  message_size (msg, &size);
	  message_lines (msg, &lines);
	  tsize += size + lines;
	  num++;
	}
    }
  pop3d_outf ("+OK %d %d\r\n", num, tsize);

  return OK;
}
