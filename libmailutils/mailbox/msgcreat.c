/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2007, 2009-2012 Free Software
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

#include <config.h>
#include <stdlib.h>

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/sys/message.h>

/*  Allocate ressources for the mu_message_t.  */
int
mu_message_create (mu_message_t *pmsg, void *owner)
{
  mu_message_t msg;
  int status;

  if (pmsg == NULL)
    return MU_ERR_OUT_PTR_NULL;
  msg = calloc (1, sizeof (*msg));
  if (msg == NULL)
    return ENOMEM;
  status = mu_monitor_create (&msg->monitor, 0, msg);
  if (status != 0)
    {
      free (msg);
      return status;
    }
  msg->owner = owner;
  msg->ref_count = 1;
  *pmsg = msg;
  return 0;
}

void *
mu_message_get_owner (mu_message_t msg)
{
  return (msg == NULL) ? NULL : msg->owner;
}
