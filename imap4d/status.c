/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

/*
 *
 */

int
imap4d_status (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *mailbox_name;
  char *data;
  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");
  mailbox_name = util_getword (arg, &sp);
  data = util_getword (NULL, &sp);
  if (!mailbox_name || !data)
    return util_finish (command, RESP_BAD, "Too few args");
  return util_finish (command, RESP_BAD, "Not supported");
}
