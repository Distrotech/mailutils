/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#include "pop3d.h"

int
pop3d_lock ()
{
  url_t url = NULL;
  locker_t lock = NULL;
  const char *name;
  int status;

  mailbox_get_url (mbox, &url);
  name = url_to_string (url);
  mailbox_get_locker (mbox, &lock);
  locker_set_flags (lock, MU_LOCKER_PID);
  if ((status = locker_lock (lock)))
    {
      syslog (LOG_NOTICE, _("mailbox '%s' lock failed: %s"),
	      (name) ? name : "?", mu_strerror(status));
      return ERR_MBOX_LOCK;
    }
  return 0;
}

int
pop3d_touchlock ()
{
  locker_t lock = NULL;
  mailbox_get_locker (mbox, &lock);
  locker_touchlock (lock);
  return 0;
}

int
pop3d_unlock ()
{
  locker_t lock = NULL;
  mailbox_get_locker (mbox, &lock);
  locker_unlock (lock);
  return 0;
}
