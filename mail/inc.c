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
 * inc[orporate]
 */

int
mail_inc (int argc, char **argv)
{
  if (!mailbox_is_updated (mbox))
    {
      mailbox_messages_count (mbox, &total);
      fprintf (ofile, "New mail has arrived\n");
    }
  else
    fprintf (ofile, "No new mail for %s\n", mail_whoami());
      
  return 0;
}
