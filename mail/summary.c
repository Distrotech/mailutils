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

/* Simple summary dysplaying a blurb on the name of the
   mailbox and how many new:deleted:read messages.
   The side effect is that is set the cursor/realcursor
   to the newest or read message number.  */
int
mail_summary (int argc, char **argv)
{
  message_t msg;
  attribute_t attr;
  size_t msgno;
  size_t count = 0;
  int mseen = 0, mnew = 0, mdelete = 0;
  int first_new = 0, first_unread = 0;

  (void)argc; (void)argv;
  mailbox_messages_count (mbox, &count);
  for (msgno = 1; msgno <= count; msgno++)
    {
      if ((mailbox_get_message (mbox, msgno, &msg) == 0)
	  && (message_get_attribute (msg, &attr) == 0))
	    {
	      int deleted = attribute_is_deleted (attr);

	      if (deleted)
		mdelete++;
	      if (attribute_is_seen (attr) && ! attribute_is_read (attr))
		{
		  mseen++;
		  if (!deleted && !first_unread)
		    first_unread = msgno;
		}
	      if (attribute_is_recent (attr))
		{
		  mnew++;
		  if (!deleted && !first_new)
		    first_new = msgno;
		}
	}
    }

  /* Print the mailbox name.  */
  {
    url_t url = NULL;
    mailbox_get_url (mbox, &url);
    printf("\"%s\": ", url_to_string (url));
  }
  printf("%d messages", count);
  if (mnew > 0)
    printf(" %d new", mnew);
  if (mseen > 0)
    printf(" %d unread", mseen);
  if (mdelete > 0)
    printf(" %d deleted", mdelete);
  printf("\n");

  /* Set the cursor.  */
  cursor = realcursor =  (first_new == 0) ? ((first_unread == 0) ?
					     1 : first_unread) : first_new ;
  return 0;
}
