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
 * h[eaders] [msglist] -- GNU extension
 */

/*
 * NOTE: check for messages near each other and only print them once
 *	 should probably build a dynamic msglist for this
 */

int
mail_headers (int argc, char **argv)
{
  char buf[64];
  int low = 1, high = total, *list = NULL, i = 0;
  int lines = util_getlines ();
  int num = util_expand_msglist (argc, argv, &list);

  lines = (lines / num) - 2;

  for (i = 0; i < num; i++)
    {
      if (lines < total)
	{
	  low = list[i] - (lines / 2);
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
      util_do_command (buf);
    }

  free (list);
  return 0;
}
