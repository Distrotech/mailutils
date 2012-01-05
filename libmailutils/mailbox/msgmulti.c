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
#include <mailutils/mime.h>
#include <mailutils/errno.h>
#include <mailutils/sys/message.h>

int
mu_message_is_multipart (mu_message_t msg, int *pmulti)
{
  if (msg && pmulti)
    {
      if (msg->_is_multipart)
	return msg->_is_multipart (msg, pmulti);
      if (msg->mime == NULL)
	{
	  int status = mu_mime_create (&msg->mime, msg, 0);
	  if (status != 0)
	    return 0;
	  msg->flags |= MESSAGE_MIME_OWNER;
	}
      *pmulti = mu_mime_is_multipart(msg->mime);
    }
  return 0;
}

int
mu_message_set_is_multipart (mu_message_t msg,
			     int (*_is_multipart) (mu_message_t, int *),
			     void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_is_multipart = _is_multipart;
  return 0;
}

