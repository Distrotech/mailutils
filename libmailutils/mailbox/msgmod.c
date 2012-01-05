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
#include <mailutils/attribute.h>
#include <mailutils/body.h>
#include <mailutils/sys/message.h>

int
mu_message_is_modified (mu_message_t msg)
{
  int mod = 0;
  if (msg)
    {
      if (mu_header_is_modified (msg->header))
	mod |= MU_MSG_HEADER_MODIFIED;
      if (mu_attribute_is_modified (msg->attribute))
	mod |= MU_MSG_ATTRIBUTE_MODIFIED;
      if (mu_body_is_modified (msg->body))
	mod |= MU_MSG_BODY_MODIFIED;
      if (msg->flags & MESSAGE_MODIFIED)
	mod |= MU_MSG_BODY_MODIFIED | MU_MSG_HEADER_MODIFIED;
    }
  return mod;
}

int
mu_message_clear_modified (mu_message_t msg)
{
  if (msg)
    {
      if (msg->header)
	mu_header_clear_modified (msg->header);
      if (msg->attribute)
	mu_attribute_clear_modified (msg->attribute);
      if (msg->body)
	mu_body_clear_modified (msg->body);
      msg->flags &= ~MESSAGE_MODIFIED;
    }
  return 0;
}
