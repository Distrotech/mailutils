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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mail.h"

/*
 * q[uit]
 * <EOF>
 */

int
mail_quit (int argc, char **argv)
{
  (void)argc; (void)argv;
  if (mail_mbox_close ())
    return 1;
  exit (0);
}

int
mail_mbox_close ()
{
  url_t url = NULL;
  size_t held_count;

  if (util_getenv (NULL, "readonly", Mail_env_boolean, 0))
    {
      if (mail_mbox_commit ())
	return 1;

      mailbox_flush (mbox, 1);
    }
  
  mailbox_get_url (mbox, &url);
  mailbox_messages_count (mbox, &held_count);
  fprintf (ofile, 
           ngettext ("Held %d message in %s\n",
                     "Held %d messages in %s\n",
                     held_count),
           held_count, url_to_string (url));
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

int
mail_mbox_commit ()
{
  unsigned int i;
  mailbox_t dest_mbox = NULL;
  int saved_count = 0;
  message_t msg;
  attribute_t attr;
  int keepsave = util_getenv (NULL, "keepsave", Mail_env_boolean, 0) == 0;
  int hold = util_getenv (NULL, "hold", Mail_env_boolean, 0) == 0;
  url_t url;
  int is_user_mbox;

  mailbox_get_url (mbox, &url);
  is_user_mbox = strcmp (url_to_string (url), getenv ("MBOX")) == 0;

  {
    mailbox_t mb;
    url_t u;
    mailbox_create_default (&mb, NULL);
    mailbox_get_url (mb, &u);
    if (strcmp (url_to_string (u), url_to_string (url)) != 0)
      {
	/* The mailbox we are closing is not a system one (%). Raise
	   hold flag */
	hold = 1;
      }
    mailbox_destroy (&mb);
  }

  for (i = 1; i <= total; i++)
    {
      if (util_get_message (mbox, i, &msg))
	return 1;

      message_get_attribute (msg, &attr);

      if (!is_user_mbox
	  && !attribute_is_deleted (attr)
	  && (attribute_is_userflag (attr, MAIL_ATTRIBUTE_MBOXED)
	      || (!hold && attribute_is_read (attr))))
	{
	  if (!dest_mbox)
	    {
	      char *name = getenv ("MBOX");

	      if (mailbox_create_default (&dest_mbox, name)
		  || mailbox_open (dest_mbox,
				   MU_STREAM_WRITE | MU_STREAM_CREAT))
		{
		  util_error (_("can't create mailbox %s"), name);
		  return 1;
		}
	    }

	  mailbox_append_message (dest_mbox, msg);
	  attribute_set_deleted (attr);
	  saved_count++;
	}
      else if (!keepsave && attribute_is_userflag (attr, MAIL_ATTRIBUTE_SAVED))
	attribute_set_deleted (attr);
      else if (attribute_is_read (attr))
	attribute_set_seen (attr);
    }

  if (saved_count)
    {
      url_t u = NULL;

      mailbox_get_url (dest_mbox, &u);
      fprintf(ofile, 
              ngettext ("Saved %d message in %s\n",
                        "Saved %d messages in %s\n",
			saved_count),
              saved_count, url_to_string (u));
      mailbox_close (dest_mbox);
      mailbox_destroy (&dest_mbox);
    }
  return 0;
}
