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
  FIXME: We need to lock the file to prevent simultaneous access.
 */

int
imap4d_subscribe (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *name;
  char *file;
  FILE *fp;

  name = util_getword (arg, &sp);
  util_unquote (&name);
  if (!name || *name == '\0')
    return util_finish (command, RESP_BAD, "Too few arguments");

  asprintf (&file, "%s/.mailboxlist", homedir);
  fp = fopen (file, "a");
  free (file);
  if (fp)
    {
      fputs (name, fp);
      fputs ("\n", fp);
      fclose (fp);
      return util_finish (command, RESP_OK, "Completed");
    }
  return util_finish (command, RESP_NO, "Can not subscribe");
}
