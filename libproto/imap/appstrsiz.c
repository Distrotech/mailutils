/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/errno.h>
#include <mailutils/imap.h>
#include <mailutils/sys/imap.h>

static int
get_crlf_stream_size (mu_stream_t str, mu_off_t size, mu_off_t *prealsize)
{
  mu_stream_t null, flt;
  mu_stream_stat_buffer stat;
  int rc;
  mu_off_t needle;
  
  rc = mu_nullstream_create (&null, MU_STREAM_WRITE);
  if (rc)
    return rc;
  mu_stream_set_stat (null, MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT), stat);
  rc = mu_filter_create (&flt, null, "CRLF", MU_FILTER_ENCODE,
			 MU_STREAM_WRITE);
  mu_stream_unref (null);
  if (rc)
    return rc;
  rc = mu_stream_seek (str, 0, MU_SEEK_CUR, &needle);
  if (rc)
    return rc;
  rc = mu_stream_copy (flt, str, size, NULL);
  if (rc == 0)
    rc = mu_stream_seek (str, 0, MU_SEEK_SET, NULL);
  mu_stream_unref (flt);
  if (rc == 0)
    *prealsize = stat[MU_STREAM_STAT_OUT];
  return rc;
}

int
mu_imap_append_stream_size (mu_imap_t imap, const char *mailbox, int flags,
			    struct tm *tm, struct mu_timezone *tz,
			    mu_stream_t stream, mu_off_t size)
{
  int status;
  mu_off_t realsize;
  
  if (imap == NULL || !mailbox || !stream || size == 0)
    return EINVAL;
  if (!imap->io)
    return MU_ERR_NO_TRANSPORT;
  if (imap->session_state < MU_IMAP_SESSION_AUTH)
    return MU_ERR_SEQ;

  switch (imap->client_state)
    {
    case MU_IMAP_CLIENT_READY:
      status = get_crlf_stream_size (stream, size, &realsize);
      if (status)
	return status;
      
      status = _mu_imap_tag_next (imap);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      status = mu_imapio_printf (imap->io, "%s APPEND ", imap->tag_str);
      MU_IMAP_CHECK_ERROR (imap, status);
      status = mu_imapio_send_qstring (imap->io, mailbox);
      MU_IMAP_CHECK_ERROR (imap, status);
      if (flags)
	{
	  status = mu_imapio_send (imap->io, " ", 1);
	  if (status == 0)
	    status = mu_imapio_send_flags (imap->io, flags);
	  MU_IMAP_CHECK_ERROR (imap, status);
	}

      if (tm)
	{
	  status = mu_imapio_send (imap->io, " ", 1);
	  if (status == 0)
	    status = mu_imapio_send_time (imap->io, tm, tz);
	  MU_IMAP_CHECK_ERROR (imap, status);
	}

      status = mu_imapio_send (imap->io, " ", 1);
      if (status == 0)
	{
	  mu_stream_t flt;
	  status = mu_filter_create (&flt, stream, "CRLF", MU_FILTER_ENCODE,
				     MU_STREAM_READ);
	  if (status == 0)
	    {
	      status = mu_imapio_send_literal_stream (imap->io, flt, realsize);
	      mu_stream_unref (flt);
	    }
	}
      MU_IMAP_CHECK_ERROR (imap, status);
      status = mu_imapio_send (imap->io, "\r\n", 2);
      MU_IMAP_CHECK_ERROR (imap, status);
      MU_IMAP_FCLR (imap, MU_IMAP_RESP);
      imap->client_state = MU_IMAP_CLIENT_APPEND_RX;

    case MU_IMAP_CLIENT_APPEND_RX:
      status = _mu_imap_response (imap, NULL, NULL);
      MU_IMAP_CHECK_EAGAIN (imap, status);
      switch (imap->response)
	{
	case MU_IMAP_OK:
	  status = 0;
	  break;

	case MU_IMAP_NO:
	  status = MU_ERR_FAILURE;
	  break;

	case MU_IMAP_BAD:
	  status = MU_ERR_BADREPLY;
	  break;
	}
      imap->client_state = MU_IMAP_CLIENT_READY;
      break;

    default:
      status = EINPROGRESS;
    }
  return status;
}

      
      

