/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/types.h>
#include <mailutils/mailbox.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/msgset.h>
#include <mailutils/sys/mailbox.h>

int
mu_mailbox_msgset_copy (mu_mailbox_t mbox, mu_msgset_t msgset,
			const char *dest, int flags)
{
  if (!mbox)
    return EINVAL;
  if (!mbox->_copy)
    return ENOSYS;
  return mbox->_copy (mbox, msgset, dest, flags);
}

int
mu_mailbox_message_copy (mu_mailbox_t mbox, size_t msgno,
			 const char *dest, int flags)
{
  int rc;
  mu_msgset_t msgset;
  int mode;
  
  if (!mbox)
    return EINVAL;
  if (!mbox->_copy)
    return ENOSYS;

  mode = flags & MU_MAILBOX_COPY_UID ? MU_MSGSET_UID : MU_MSGSET_NUM;
  rc = mu_msgset_create (&msgset, mbox, mode);
  if (rc)
    return rc;
  rc = mu_msgset_add_range (msgset, 1, 1, mode);
  if (rc == 0)
    rc = mbox->_copy (mbox, msgset, dest, flags);
  mu_msgset_destroy (&msgset);
  return rc;
}
