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
 * q[uit]
 * <EOF>
 */

int
mail_quit (int argc, char **argv)
{
  if (mail_mbox_close ())
    return 1;
  exit (0);
}

int
mail_mbox_close ()
{
  url_t url = NULL;
  size_t held_count;
  
  if (mail_mbox_commit ())
    return 1;
  
  mailbox_expunge (mbox);

  mailbox_get_url (mbox, &url);
  mailbox_messages_count (mbox, &held_count);
  fprintf (ofile, "Held %d messages in %s\n", held_count, url_to_string (url));
  
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
  return 0;
}

