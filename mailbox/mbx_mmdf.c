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

#include <mailbox0.h>
#include <registrar0.h>

#include <errno.h>

static int mailbox_mmdf_create (mailbox_t *mbox, const char *name);
static void mailbox_mmdf_destroy (mailbox_t *mbox);

struct mailbox_registrar _mailbox_mmdf_registrar =
{
  "MMDF",
  mailbox_mmdf_create, mailbox_mmdf_destroy
};

static int
mailbox_mmdf_create (mailbox_t *mbox, const char *name)
{
  (void)mbox; (void)name;
  return ENOSYS;
}

static void
mailbox_mmdf_destroy (mailbox_t *mbox)
{
  (void)mbox;
  return;
}
