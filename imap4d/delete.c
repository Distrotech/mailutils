/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2007 Free Software Foundation, Inc.

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

/*
 *  
 */

int
imap4d_delete (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  int rc = RESP_OK;
  const char *msg = "Completed";
  const char *delim = "/";
  char *name;

  name = util_getword (arg, &sp);
  util_unquote (&name);
  if (!name || *name == '\0')
    return util_finish (command, RESP_BAD, "Too few arguments");

  /* It is an error to attempt to delele "INBOX or a mailbox
     name that dos not exists.  */
  if (strcasecmp (name, "INBOX") == 0)
    return util_finish (command, RESP_NO, "Already exist");

 /* Allocates memory.  */
  name = namespace_getfullpath (name, delim);
  if (!name)
    return util_finish (command, RESP_NO, "Can not remove");

  if (remove (name) != 0)
    {
      rc = RESP_NO;
      msg = "Can not remove";
    }
  free (name);
  return util_finish (command, rc, msg);
}
