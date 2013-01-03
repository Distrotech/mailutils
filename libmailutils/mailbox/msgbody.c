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
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/stream.h>
#include <mailutils/sys/message.h>

int
mu_message_get_body (mu_message_t msg, mu_body_t *pbody)
{
  if (msg == NULL)
    return EINVAL;
  if (pbody == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* Is it a floating mesg.  */
  if (msg->body == NULL)
    {
      mu_body_t body;
      int status = mu_body_create (&body, msg);
      if (status != 0)
	return status;
      /* If a stream is already set, use it to create the body stream.  */
      if (msg->rawstream)
	{
	  mu_stream_t stream;
	  int flags = 0;

	  /* FIXME: The actual mu_header_size cannot be used as offset,
	     because the headers might have been modified in between. */
	  
	  mu_stream_get_flags (msg->rawstream, &flags);
	  status = mu_streamref_create_abridged (&stream, msg->rawstream,
						 msg->orig_header_size, 0);
	  if (status)
	    {
	      mu_body_destroy (&body, msg);
	      return status;
	    }
	  mu_body_set_stream (body, stream, msg);
	}
      msg->body = body;
    }
  *pbody = msg->body;
  return 0;
}

int
mu_message_set_body (mu_message_t msg, mu_body_t body, void *owner)
{
  if (msg == NULL )
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  /* Make sure we destroy the old if it was owned by the mesg.  */
  /* FIXME:  I do not know if somebody has already a ref on this ? */
  if (msg->body)
    mu_body_destroy (&msg->body, msg);
  msg->body = body;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}
