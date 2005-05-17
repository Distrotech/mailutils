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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "mail.h"

/*
 * hel[p] [command]
 * ? [command]
 */

int
mail_help (int argc, char **argv)
{
  if (argc < 2)
    return util_help (mail_command_table, NULL);
  else
    {
      int status = 0;

      while (--argc)
	status |= util_help (mail_command_table, *++argv);

      return status;
    }
  return 1;
}
