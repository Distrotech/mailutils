/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002 Free Software Foundation, Inc.

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
 * una[lias] [alias]...
 */

int
mail_unalias (int argc, char **argv)
{
  if (argc == 1)
    {
      /* TRANSLATORS: 'unalias' is a command name. Do not translate it! */
      util_error (_("unalias requires at least one argument"));
      return 1;
    }
  while (--argc)
    alias_destroy (*++argv);
  return 0;
}
