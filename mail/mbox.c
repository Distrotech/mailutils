/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "mail.h"

/*
 * mb[ox] [msglist]
 */

static int
mbox0 (msgset_t *mspec, message_t msg, void *data)
{
  attribute_t attr;

  message_get_attribute (msg, &attr);
  attribute_set_userflag (attr, MAIL_ATTRIBUTE_MBOXED);
  util_mark_read (msg);
  
  cursor = mspec->msg_part[0];
  
  return 0;
}

int
mail_mbox (int argc, char **argv)
{
  return util_foreach_msg (argc, argv, MSG_NODELETED, mbox0, NULL);
}
