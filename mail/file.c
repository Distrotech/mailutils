/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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
 * fi[le] [file]
 * fold[er] [file]
 */

int
mail_file (int argc, char **argv)
{
  if (argc == 1)
    {
      /* display current folder info */
    }
  else if (argc == 2)
    {
      /* switch folders */
      /*
       * special characters:
       * %	system mailbox
       * %user	system mailbox of user
       * #	the previous file
       * &	the current mbox
       * +file	the file named in the folder directory (set folder=foo)
       */
      mailbox_t newbox;
      if (mailbox_create (&newbox, argv[1]) != 0)
	return 1;
      if (mailbox_open (newbox, MU_STREAM_READ) != 0)
	return 1;
      mailbox_messages_count (newbox, &total);
      mailbox_expunge (mbox);
      mailbox_close (mbox);
      mbox = newbox;
      /* mailbox_destroy (&newbox); */
      if ((util_find_env("header"))->set)
	util_do_command ("from *");
      return 0;
    }
  return 1;
}
