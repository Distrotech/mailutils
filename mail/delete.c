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
 * d[elete] [msglist]
 */

static int
mail_delete_msg (msgset_t *mspec, message_t msg, void *data)
{
  attribute_t attr;

  message_get_attribute (msg, &attr);
  attribute_set_deleted (attr);

  if (cursor == mspec->msg_part[0])
    {
      /* deleting current message. let the caller know */
      *(int *)data = 1;
    }
  return 0;
}

int
mail_delete (int argc, char **argv)
{
  int reset_cursor = 0;
  int rc = util_foreach_msg (argc, argv, MSG_NODELETED,
			     mail_delete_msg, &reset_cursor);
  
  /* Readjust the cursor not to point to the deleted messages.  */
  if (reset_cursor)
    {
      unsigned int here;
      for (here = cursor; here <= total; here++)
	{
	  message_t msg = NULL;
	  attribute_t attr = NULL;
	  
	  mailbox_get_message (mbox, here, &msg);
	  message_get_attribute (msg, &attr);
	  if (!attribute_is_deleted (attr))
	    break;
	}

      if (here > total)
	for (here = cursor; here > 0; here--)
	  {
	    message_t msg = NULL;
	    attribute_t attr = NULL;
	  
	    mailbox_get_message (mbox, here, &msg);
	    message_get_attribute (msg, &attr);
	    if (!attribute_is_deleted (attr))
	      break;
	  }
      cursor = here;
    }

  if (util_getenv (NULL, "autoprint", Mail_env_boolean, 0) == 0)
    util_do_command("print");

  return rc;
}

