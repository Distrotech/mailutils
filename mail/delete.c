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
 * d[elete] [msglist]
 */

static int
mail_delete0 (void)
{
  message_t msg;
  attribute_t attr;

  if (util_get_message (mbox, cursor, &msg, 0))
    return 1;
  message_get_attribute (msg, &attr);
  attribute_set_deleted (attr);
  return 0;
}

int
mail_delete (int argc, char **argv)
{
  int rc = 0;

  if (argc > 1)
    rc = util_msglist_command (mail_delete, argc, argv, 0);
  else
    rc = mail_delete0 ();

  /* Readjust the realcursor to no point to the deleted messages.  */
  if (cursor == realcursor)
    {
      unsigned int here = realcursor;
      do
	{
	  message_t msg = NULL;
	  attribute_t attr = NULL;

	  mailbox_get_message (mbox, realcursor, &msg);
	  message_get_attribute (msg, &attr);
	  if (!attribute_is_deleted (attr))
	    break;
	  if (++realcursor > total)
	    realcursor = 1;
	}
      while (realcursor != here);

      cursor = realcursor;
    }

  if (util_getenv (NULL, "autoprint", Mail_env_boolean, 0) == 0)
    util_do_command("print");

  return rc;
}

