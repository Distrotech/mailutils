/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2005 Free Software Foundation, Inc.

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
 * n[ext] [message]
 * +
 */

int
mail_next (int argc, char **argv)
{
  size_t n;
  message_t msg;
  
  if (argc < 2)
    {
      int rc;
      attribute_t attr = NULL;
      
      rc = util_get_message (mbox, cursor, &msg);
      if (rc)
	{
	  util_error (_("No applicable message"));
	  return 1;
	}

      message_get_attribute (msg, &attr);
      if (!attribute_is_userflag (attr, MAIL_ATTRIBUTE_SHOWN))
	{
	  util_do_command ("print");
	  return 0;
	}
      
      rc = 1;
      for (n = cursor + 1; n <= total; n++)
	{
	  if (util_isdeleted (n))
	    continue;
	  rc = util_get_message (mbox, n, &msg);
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
      int rc = msgset_parse (argc, argv, MSG_NODELETED|MSG_SILENT, &list);
      if (!rc)
	{
	  n = list->msg_part[0];
	  msgset_free (list);
	  if (util_get_message (mbox, n, &msg))
	    return 1;
	}
      else
	{
	  util_error (_("No applicable message"));
	  return 1;
	}
    }
  cursor = n;
  util_do_command ("print");
  return 0;
}
