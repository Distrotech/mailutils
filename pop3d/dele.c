/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

/* DELE adds a message number to the list of messages to be deleted on QUIT */

int
pop3_dele (const char *arg)
{
  int num = 0;
  message_t msg;
  attribute_t attr;

  if ((arg == NULL) || (strchr (arg, ' ') != NULL))
    return ERR_BAD_ARGS;

  if (state != TRANSACTION)
    return ERR_WRONG_STATE;

  num = atoi (arg);

  if (mailbox_get_message (mbox, num, &msg) != 0)
    return ERR_NO_MESG;

  message_get_attribute (msg, &attr);
  attribute_set_deleted (attr);
  fprintf (ofile, "+OK Message %d marked\r\n", num);
  return OK;
}
