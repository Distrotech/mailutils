/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/error.h>
#include <mailutils/sys/mbox.h>

int
mbox_set_progress_cb (mbox_t mbox, int (*cb) __P ((int, void *)), void *arg)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  mbox->progress.cb = cb;
  mbox->progress.arg = arg;
  return 0;
}

int
mbox_set_newmsg_cb (mbox_t mbox, int (*cb) __P ((int, void *)), void *arg)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  mbox->newmsg.cb = cb;
  mbox->newmsg.arg = arg;
  return 0;
}

int
mbox_set_error_cb (mbox_t mbox, int (*cb) __P ((int, void *)), void *arg)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  mbox->error.cb = cb;
  mbox->error.arg = arg;
  return 0;
}

int
mbox_newmsg_cb (mbox_t mbox, int info)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  if (mbox->newmsg.cb)
    return mbox->newmsg.cb (info, mbox->newmsg.arg);
  return 0;
}

int
mbox_progress_cb (mbox_t mbox, int info)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  if (mbox->progress.cb)
    return mbox->progress.cb (info, mbox->progress.arg);
  return  0;
}

int
mbox_error_cb (mbox_t mbox, int info)
{
  if (mbox == NULL)
    return MU_ERROR_INVALID_PARAMETER;
  if (mbox->error.cb)
    return mbox->error.cb (info, mbox->error.arg);
  return  0;
}
