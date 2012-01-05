/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <mailutils/errno.h>
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>
#include <mailutils/tls.h>

int
_mu_smtp_get_streams (mu_smtp_t smtp, mu_stream_t *streams)
{
  int rc;
  
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_TRACE))
    rc = mu_stream_ioctl (smtp->carrier, MU_IOCTL_SUBSTREAM, MU_IOCTL_OP_GET,
                          streams);
  else
    {
      streams[0] = smtp->carrier;
      mu_stream_ref (streams[0]);
      streams[1] = smtp->carrier;
      mu_stream_ref (streams[1]);
      rc = 0;
    }
  return rc;
}

int
_mu_smtp_set_streams (mu_smtp_t smtp, mu_stream_t *streams)
{
  int rc;
  
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_TRACE))
    rc = mu_stream_ioctl (smtp->carrier, MU_IOCTL_SUBSTREAM, MU_IOCTL_OP_SET,
                          streams);
  else
    {
      mu_stream_t tmp;
      
      if (streams[0] == streams[1])
	{
	  tmp = streams[0];
	  mu_stream_ref (tmp);
	  mu_stream_ref (tmp);
	  rc = 0;
	}
      else
	rc = mu_iostream_create (&tmp, streams[0], streams[1]);
      if (rc == 0)
	{
	  mu_stream_unref (smtp->carrier);
	  smtp->carrier = tmp;
	}
    }
  return rc;
}

int
mu_smtp_starttls (mu_smtp_t smtp)
{
#ifdef WITH_TLS
  int status;
  mu_stream_t tlsstream, streams[2];
  
  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_TLS))
    return MU_ERR_SEQ;
  if (smtp->state != MU_SMTP_MAIL)
    return MU_ERR_SEQ;

  status = mu_smtp_write (smtp, "STARTTLS\r\n");
  MU_SMTP_CHECK_ERROR (smtp, status);
  status = mu_smtp_response (smtp);
  MU_SMTP_CHECK_ERROR (smtp, status);
  if (smtp->replcode[0] == '4')
    return EAGAIN;
  else if (smtp->replcode[0] != '2')
    return MU_ERR_FAILURE;

  mu_stream_flush (smtp->carrier);
  status = _mu_smtp_get_streams (smtp, streams);
  MU_SMTP_CHECK_ERROR (smtp, status);
  
  status = mu_tls_client_stream_create (&tlsstream,
					streams[0], streams[1], 0);
  mu_stream_unref (streams[0]);
  mu_stream_unref (streams[1]);
  MU_SMTP_CHECK_ERROR (smtp, status);
  streams[0] = streams[1] = tlsstream;
  status = _mu_smtp_set_streams (smtp, streams);
  mu_stream_unref (streams[0]);
  mu_stream_unref (streams[1]);
  MU_SMTP_CHECK_ERROR (smtp, status);
  /* Invalidate the capability list */
  mu_list_destroy (&smtp->capa);
  return 0;
#else
  return ENOSYS;
#endif
}


  
    
