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
 * mb[ox] [msglist]
 */

int
mail_mbox (int argc, char **argv)
{
  message_t msg;
  attribute_t attr;
  
  if (argc > 1)
    return util_msglist_command (mail_mbox, argc, argv, 1);
  else
    {
      if (mailbox_get_message (mbox, cursor, &msg))
	{
	  fprintf (ofile, "%d: can't get message\n", cursor);
	  return 1;
	}
      /* Mark the message */
      message_get_attribute (msg, &attr);
      attribute_set_userflag (attr, MAIL_ATTRIBUTE_MBOXED);
    }
  return 0;
}
  

