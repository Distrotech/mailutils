/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <url_pop.h>
#include <mbx_pop.h>
#include <errno.h>

struct mailbox_type _mailbox_pop_type =
{
  "POP3",
  (int)&_url_pop_type, &_url_pop_type,
  mailbox_pop_init, mailbox_pop_destroy
};

void
mailbox_pop_destroy (mailbox_t *mbox)
{
  return;
}

int
mailbox_pop_init (mailbox_t *mbox, const char *name)
{
  return ENOSYS;
}
