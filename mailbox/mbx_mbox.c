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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <url_mbox.h>
#include <mbx_mbox.h>
#include <mbx_unix.h>
#include <mbx_mdir.h>

struct mailbox_type _mailbox_mbox_type =
{
  "UNIX MBOX",
  (int)&_url_mbox_type, &_url_mbox_type,
  mailbox_mbox_init, mailbox_mbox_destroy
};

/*
  There is no specific URL for file mailbox,  until we
  come up with a url for each like :
  maildir://
  mmdf://
  ubox://
  they all share the same url which is
  file://<path_name> */
int
mailbox_mbox_init (mailbox_t *mbox, const char *name)
{
  return -1;
}

void
mailbox_mbox_destroy (mailbox_t *mbox)
{
  return ;
}
