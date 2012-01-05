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
#include <mailutils/observer.h>
#include <mailutils/sys/message.h>

int
mu_message_get_observable (mu_message_t msg, mu_observable_t *pobservable)
{
  if (msg == NULL || pobservable == NULL)
    return EINVAL;

  if (msg->observable == NULL)
    {
      int status = mu_observable_create (&msg->observable, msg);
      if (status != 0)
	return status;
    }
  *pobservable = msg->observable;
  return 0;
}
