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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

/*
 * prev[ious] [message]
 * -
 */

int
mail_previous (int argc, char **argv)
{
  size_t n;
  message_t msg;

  if (argc < 2)
    {
      int rc = 1;
      for (n = cursor - 1; n > 0; n--)
	{
	  rc = util_get_message (mbox, n, &msg, MSG_NODELETED|MSG_SILENT);
	  if (rc == 0)
	    break;
	}

      if (rc)
	{
	  util_error (_("No applicable message"));
	  return 1;
	}
    }
  else
    {
      msgset_t *list = NULL;
      msgset_parse (argc, argv, &list);
      n = list->msg_part[0];
      msgset_free (list);
      if (util_get_message (mbox, n, &msg, MSG_NODELETED|MSG_SILENT))
	return 1;
    }
  cursor = realcursor = n;
  util_do_command ("print");
  return 0;
}
