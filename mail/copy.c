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
 * c[opy] [file]
 * c[opy] [msglist] file
 * C[opy] [msglist]
 */

/*
 * NOTE: outfolder variable
 */

int
mail_copy (int argc, char **argv)
{
  message_t msg;
  mailbox_t mbx;
  char *filename = NULL;
  int *msglist = NULL;
  int num = 0, i = 0;
  int sender = 0;
  
  if (isupper (argv[0][0]))
    sender = 1;
  else if (argc >= 2)
    filename = argv[--argc];
  else
    {
      filename = strdup ("mbox");
    }

  num = util_expand_msglist (argc, argv, &msglist);

  if (sender)
    {
      mailbox_get_message (mbox, num > 0 ? msglist[0] : cursor, 0);
      /* get from */
      /* filename = login name part */
    }

  mailbox_create (&mbx, filename);
  mailbox_open (mbx, MU_STREAM_WRITE | MU_STREAM_CREAT);
  
  if (num > 0)
    {
      for (i=0; i < num; i++)
	{
	  mailbox_get_message (mbox, msglist[i], &msg);
	  mailbox_append_message (mbx, msg);
	}
    }
  else
    {
      mailbox_get_message (mbox, cursor, &msg);
      mailbox_append_message (mbx, msg);
    }

  mailbox_close (mbx);
  mailbox_destroy (&mbx);

  free (msglist);
  return 0;
}
