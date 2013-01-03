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
#include <string.h>

#include <mailutils/types.h>
#include <mailutils/message.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/stream.h>
#include <mailutils/sys/message.h>


enum eoh_state
  {
    eoh_no,
    eoh_maybe,
    eoh_yes
  };

/* Message header stuff */
static enum eoh_state
string_find_eoh (enum eoh_state eoh, const char *str, size_t len,
		 size_t *ppos)
{
  size_t pos;

  if (eoh == eoh_maybe && *str == '\n')
    {
      *ppos = 0;
      return eoh_yes;
    }
  
  for (pos = 0; pos < len - 1; pos++)
    if (str[pos] == '\n' && str[pos + 1] == '\n')
      {
	*ppos = pos + 1;
	return eoh_yes;
      }
  
  *ppos = pos + 1;
  return str[pos] == '\n' ? eoh_maybe : eoh_no;
}

#define MIN_HEADER_BUF_SIZE 2048

static int
_header_fill (mu_stream_t stream, char **pbuf, size_t *plen)
{
  int status = 0;
  char *buffer = NULL;
  size_t bufsize = 0;
  char inbuf[MIN_HEADER_BUF_SIZE];
  size_t nread;
  enum eoh_state eoh = eoh_no;
  
  status = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  if (status)
    return status;
      
  while (eoh != eoh_yes
	 && (status = mu_stream_read (stream, inbuf, sizeof (inbuf), &nread))
	    == 0
	 && nread)
    {
      char *nbuf;
      size_t len;

      eoh = string_find_eoh (eoh, inbuf, nread, &len);
      
      nbuf = realloc (buffer, bufsize + len);
      if (!nbuf)
	{
	  status = ENOMEM;
	  break;
	}
      memcpy (nbuf + bufsize, inbuf, len);
      buffer = nbuf;
      bufsize += len;
    }

  if (status)
    free (buffer);
  else
    {
      *pbuf = buffer;
      *plen = bufsize;
    }
  return status;
}

static int
message_header_fill (void *data, char **pbuf, size_t *plen)
{
  mu_message_t msg = data;

  return _header_fill (msg->rawstream, pbuf, plen);
}

int
mu_message_get_header (mu_message_t msg, mu_header_t *phdr)
{
  if (msg == NULL)
    return EINVAL;
  if (phdr == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (msg->header == NULL)
    {
      mu_header_t header;
      int status = mu_header_create (&header, NULL, 0);
      if (status != 0)
	return status;
      if (msg->rawstream)
	mu_header_set_fill (header, message_header_fill, msg);
      status = mu_header_size (header, &msg->orig_header_size);
      if (status)
	return status;
      msg->header = header;
    }
  *phdr = msg->header;
  return 0;
}

/* Note: mu_message_set_header steals the reference to hdr */
int
mu_message_set_header (mu_message_t msg, mu_header_t hdr, void *owner)
{
  if (msg == NULL )
    return EINVAL;
  if (msg->owner != owner)
     return EACCES;
  if (msg->header)
    mu_header_destroy (&msg->header);
  msg->header = hdr;
  msg->flags |= MESSAGE_MODIFIED;
  return 0;
}
