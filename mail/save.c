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
 * s[ave] [file]
 * s[ave] [msglist] file
 * S[ave] [msglist]
 */

int
mail_save (int argc, char **argv)
{
  message_t msg;
  int *msglist = NULL;
  int num = 0, i = 0;

  mail_copy (argc, argv);

  if (argc > 1)
  argc--;

  num = util_expand_msglist (argc, argv, &msglist);
  if (num > 0)
    {
      for (i = 0; i < num; i++)
	{
	  mailbox_get_message (mbox, msglist[i], &msg);
	  /* mark as saved */
	}
    }
  else
    {
      mailbox_get_message (mbox, cursor, &msg);
    }
  free (msglist);
  return 0;
}
