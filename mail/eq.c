/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003 Free Software Foundation, Inc.

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
 * =
 */

int
mail_eq (int argc, char **argv)
{
  msgset_t *list = NULL;
    
  switch (argc)
    {
    case 1:
      fprintf (ofile, "%d\n", cursor);
      break;

    case 2:
      if (msgset_parse (argc, argv, MSG_NODELETED, &list) == 0)
	{
	  if (list->msg_part[0] <= total)
	    {
	      cursor = list->msg_part[0];
	      fprintf (ofile, "%d\n", cursor);
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
