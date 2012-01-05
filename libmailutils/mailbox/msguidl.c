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
#include <time.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/stream.h>
#include <mailutils/md5.h>
#include <mailutils/sys/message.h>

int
mu_message_get_uidl (mu_message_t msg, char *buffer, size_t buflen,
		     size_t *pwriten)
{
  mu_header_t header = NULL;
  size_t n = 0;
  int status;

  if (msg == NULL || buffer == NULL || buflen == 0)
    return EINVAL;

  buffer[0] = '\0';
  /* Try the function overload if error fallback.  */
  if (msg->_get_uidl)
    {
      status = msg->_get_uidl (msg, buffer, buflen, pwriten);
      if (status == 0)
	return status;
    }

  /* Be compatible with Qpopper ? qppoper saves the UIDL in "X-UIDL".
     We generate a chksum and save it in the header.  */
  mu_message_get_header (msg, &header);
  status = mu_header_get_value_unfold (header, "X-UIDL", buffer, buflen, &n);
  if (status != 0 || n == 0)
    {
      size_t uid = 0;
      struct mu_md5_ctx md5context;
      mu_stream_t stream = NULL;
      char buf[1024];
      unsigned char md5digest[16];
      char *tmp;
      n = 0;
      mu_message_get_uid (msg, &uid);
      mu_message_get_streamref (msg, &stream);
      mu_md5_init_ctx (&md5context);
      status = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
      if (status == 0)
	{
	  while (mu_stream_read (stream, buf, sizeof (buf), &n) == 0
		 && n > 0)
	    mu_md5_process_bytes (buf, n, &md5context);
	  mu_md5_finish_ctx (&md5context, md5digest);
	  tmp = buf;
	  for (n = 0; n < 16; n++, tmp += 2)
	    sprintf (tmp, "%02x", md5digest[n]);
	  *tmp = '\0';
	  /* POP3 rfc says that an UID should not be longer than 70.  */
	  snprintf (buf + 32, 70, ".%lu.%lu", (unsigned long)time (NULL), 
		    (unsigned long) uid);

	  mu_header_set_value (header, "X-UIDL", buf, 1);
	  buflen--; /* leave space for the NULL.  */
	  strncpy (buffer, buf, buflen)[buflen] = '\0';
	}
      mu_stream_destroy (&stream);
    }
  return status;
}

int
mu_message_set_uidl (mu_message_t msg,
		  int (* _get_uidl) (mu_message_t, char *, size_t, size_t *),
		  void *owner)
{
  if (msg == NULL)
    return EINVAL;
  if (msg->owner != owner)
    return EACCES;
  msg->_get_uidl = _get_uidl;
  return 0;
}
