/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mbx_imap.h>


struct mailbox_type _mailbox_imap_type =
{
  "IMAP",
  (int)&_url_imap_type, &_url_imap_type,
  mailbox_imap_init, mailbox_imap_destroy
};

int
mailbox_imap_destroy (mailbox_t *mbox)
{
  return;
}

int
mailbox_imap_init (mailbox_t *mbox, const char *name)
{
  return -1;
}
