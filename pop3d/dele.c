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

/* DELE adds a message number to the list of messages to be deleted on QUIT */

int
pop3d_dele (const char *arg)
{
  size_t num;
  message_t msg;
  attribute_t attr = NULL;

  if ((arg == NULL) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  num = strtoul (arg, NULL, 10);

  if (mailbox_get_message (mbox, num, &msg) != 0)
    return ERR_NO_MESG;

  message_get_attribute (msg, &attr);
  attribute_set_userflag (attr, POP3_ATTRIBUTE_DELE);
  pop3d_outf ("+OK Message %d marked\r\n", num);
  return OK;
}
