/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#include "pop3d.h"

/* EXPIRE see RFC2449:

  Implementation:
    When a message is downloaded it is mark, pop3d_mark_retr(attr) 
    at the update state the expire date is set on a header field
     asprintf (.. "X-Expire-Timestamp: %ld", (long)time(NULL));
    The message is not actually remove, we rely on external
    program to do it.
 */

void
pop3d_mark_retr (attribute_t attr)
{
  attribute_set_userflag (attr, POP3_ATTRIBUTE_RETR);
}
 
int
pop3d_is_retr (attribute_t attr)
{
  return attribute_is_userflag (attr, POP3_ATTRIBUTE_RETR);
}
 
void
pop3d_unmark_retr (attribute_t attr)
{
  if (attribute_is_userflag (attr, POP3_ATTRIBUTE_RETR))
    attribute_unset_userflag (attr, POP3_ATTRIBUTE_RETR);
}
