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

#include "mail.h"

/*
 * e[dit] [msglist]
 */

int
mail_edit (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_edit, argc, argv, 1);
  else
    {
      char *file = tempnam(getenv("TMPDIR"), "mu");
      util_do_command ("copy %s", file);
      util_do_command ("shell %s %s", getenv("EDITOR"), file);
      remove (file);
      free (file);
      return 0;
    }
  return 1;
}
