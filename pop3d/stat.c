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

#include "pop3d.h"

/* Prints the number of messages and the total size of all messages */

int
pop3_stat (const char *arg)
{
  size_t mesgno;
  size_t size = 0;
  size_t lines = 0;
  size_t total = 0;
  size_t num = 0;
  size_t tsize = 0;
  message_t msg;
  attribute_t attr;
  
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;
  
  if (state != TRANSACTION)
    return ERR_WRONG_STATE;
  
  mailbox_messages_count (mbox, &total);
  for (mesgno = 1; mesgno <= total; mesgno++)
    {
      mailbox_get_message (mbox, mesgno, &msg);
      message_get_attribute (msg, &attr);
      if (!attribute_is_deleted (attr))
	{
	  message_size (msg, &size);
	  message_lines (msg, &lines);
	  tsize += size + lines;
	  num++;
	}
    }
  fprintf (ofile, "+OK %d %d\r\n", num, tsize);
  
  return OK;
}


#if 0

int
pop3_stat (const char *arg)
{
  unsigned int count = 0;
  off_t size = 0;

  mailbox_size (mbox, &size);
  mailbox_messages_count (mbox, &count);

  fprintf (ofile, "+OK %d %d\r\n", count, (int)size);
  return OK;
}

#endif 
