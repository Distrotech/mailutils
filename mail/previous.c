/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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
 * prev[ious] [message]
 * -
 */

int
mail_previous (int argc, char **argv)
{
  if (argc < 2)
    {
      cursor--;
      realcursor--;
      return 0;
    }
  else if ( argc == 2)
    {
      cursor = strtol (argv[1], NULL, 10);
      realcursor = cursor;
      return 0;
    }
  return 1;
}
