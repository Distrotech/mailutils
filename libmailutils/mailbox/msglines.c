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
#include <mailutils/sys/message.h>

int
mu_message_lines (mu_message_t msg, size_t *plines)
{
  size_t hlines, blines;
  int ret = 0;

  if (msg == NULL)
    return EINVAL;
  /* Overload.  */
  if (msg->_lines)
    return msg->_lines (msg, plines, 0);
  if (plines)
    {
      mu_header_t hdr = NULL;
      mu_body_t body = NULL;

      hlines = blines = 0;
      mu_message_get_header (msg, &hdr);
      mu_message_get_body (msg, &body);
      if ( ( ret = mu_header_lines (hdr, &hlines) ) == 0 )
	      ret = mu_body_lines (body, &blines);
      *plines = hlines + blines;
    }
  return ret;
}

int
mu_message_set_lines (mu_message_t msg, int (*_lines)
		      (mu_message_t, size_t *, int), void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_lines = _lines;
  return 0;
}
