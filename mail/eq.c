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

#include "mail.h"

/*
 * =
 */

int
mail_eq (int argc, char **argv)
{
  msgset_t *list = NULL;
    
  switch (argc)
    {
    case 1:
      fprintf (ofile, "%d\n", realcursor);
      break;

    case 2:
      if (msgset_parse (argc, argv, &list) == 0)
	{
	  if (list->msg_part[0] <= total)
	    {
	      realcursor = cursor = list->msg_part[0];
	      fprintf (ofile, "%d\n", realcursor);
	    }
	  else
	    util_error_range (list->msg_part[0]);
	  msgset_free (list);
	}
      break;

    default:
      return 1;
    }
  
  return 0;
}
