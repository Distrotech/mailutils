/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
 * u[ndelete] [msglist]
 */

int
mail_undelete (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_undelete, argc, argv);
  else
    {
      message_t msg;
      attribute_t attr;
      if (mailbox_get_message (mbox, cursor, &msg) != 0)
        {
	  fprintf (stderr, "Meessage %d does not exist\n", cursor);
          return 1;
        }
      message_get_attribute (msg, &attr);
      if (attribute_is_deleted (attr))
	attribute_unset_deleted (attr);
      return 0;
    }
  return 1;
}
