/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
 * v[isual] [msglist]
 */

int
mail_visual (int argc, char **argv)
{
  if (argc > 1)
    return util_msglist_command (mail_visual, argc, argv, 1);
  else
    {
      message_t msg = NULL;
      attribute_t attr = NULL;
      char *file = mu_tempname (NULL);

      util_do_command ("copy %s", file); 
      util_do_command ("shell %s %s", getenv("VISUAL"), file);

      remove (file);
      free (file);

      /* Mark as read */
      mailbox_get_message (mbox, cursor, &msg);
      message_get_attribute (msg, &attr);
      attribute_set_read (attr);

      return 0;
    }
  return 1;
}
