/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
  int low = 1, high = total;
  msgset_t *list = NULL, *mp;
  int lines = util_screen_lines ();
  int num;

  if (msgset_parse (argc, argv, &list))
     return 1;

  num = 0;
  for (mp = list; mp; mp = mp->next)
     num++;

  if (num == 0)
     return 0;
  lines = (lines / num) - 2;
  if (lines < 0)
    lines = util_screen_lines ();

  if ((unsigned int)lines < total)
    {
      low = list->msg_part[0] - (lines / 2);
      if (low < 1)
	low = 1;
      high = low + util_screen_lines ();
      if ((unsigned int)high > total)
	{
	  high = total;
	  low = high - lines;
	}
    }

  util_range_msg (low, high, MSG_NODELETED|MSG_SILENT, mail_from0, NULL);

  msgset_free (list);
  return 0;
}
