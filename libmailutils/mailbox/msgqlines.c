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

/* Return the number of lines in the message, without going into
   excess trouble for calculating it.  If obtaining the result
   means downloading the entire message (as is the case for POP3,
   for example), return MU_ERR_INFO_UNAVAILABLE. */
int
mu_message_quick_lines (mu_message_t msg, size_t *plines)
{
  size_t hlines, blines;
  int rc;
  
  if (msg == NULL)
    return EINVAL;
  /* Overload.  */
  if (msg->_lines)
    {
      int rc = msg->_lines (msg, plines, 1);
      if (rc != ENOSYS)
	return rc;
    }
  if (plines)
    {
      mu_header_t hdr = NULL;
      mu_body_t body = NULL;

      hlines = blines = 0;
      mu_message_get_header (msg, &hdr);
      mu_message_get_body (msg, &body);
      if ((rc = mu_header_lines (hdr, &hlines)) == 0)
	rc = mu_body_lines (body, &blines);
      if (rc == 0)
	*plines = hlines + blines;
    }
  return rc;
}
