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

#include "mail.h"

/*
 * h[eaders] [message]
 */

int
mail_headers (int argc, char **argv)
{
  char buf[64];
  int low = 1, high = total, middle = cursor;
  int lines = util_getlines () - 2;
  
  if (argc > 2)
    return 1;
  
  if (argc == 2)
    middle = strtol (argv[1], NULL, 10);

  if (lines < total)
    {
      low = middle - (lines / 2);
      if (low < 1)
	low = 1;
      high = low + lines;
      if (high > total)
	{
	  high = total;
	  low = high - lines;
	}
    }
    
  memset (buf, '\0', 64);
  snprintf (buf, 64, "from %d-%d", low, high);
  return util_do_command (buf);
}
