/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include "mail.h"

/*
 * cd [directory]
 * ch[dir] [directory]
 */

int
mail_cd (int argc, char **argv)
{
  if (argc > 2)
    return 1;
  else if (argc == 2 && (chdir (argv[1]) != 0))
    return 1;
  else if (argc < 2 && chdir (getenv ("HOME")) != 0)
    return 1;
  return 0;
}
