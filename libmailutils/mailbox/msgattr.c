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
#include <mailutils/attribute.h>
#include <mailutils/sys/message.h>

int
mu_message_get_attribute (mu_message_t msg, mu_attribute_t *pattribute)
{
  if (msg == NULL)
    return EINVAL;
  if (pattribute == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (msg->attribute == NULL)
    {
      mu_attribute_t attribute;
      int status = mu_attribute_create (&attribute, msg);
      if (status != 0)
	return status;
      msg->attribute = attribute;
    }
  *pattribute = msg->attribute;
  return 0;
}

int
mu_message_set_attribute (mu_message_t msg, mu_attribute_t attribute, void *owner)
{
  if (msg == NULL)
   return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  if (msg->attribute)
    mu_attribute_destroy (&msg->attribute, owner);
  msg->attribute = attribute;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}
