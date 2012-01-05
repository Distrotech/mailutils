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
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/diag.h>
#include <mailutils/filter.h>
#include <mailutils/list.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/smtp.h>

int
_mu_smtp_data_begin (mu_smtp_t smtp)
{
  int status;
  
  status = mu_smtp_write (smtp, "DATA\r\n");
  MU_SMTP_CHECK_ERROR (smtp, status);
  status = mu_smtp_response (smtp);
  MU_SMTP_CHECK_ERROR (smtp, status);
  
  if (smtp->replcode[0] != '3')
    return MU_ERR_REPLY;
  
  if (mu_smtp_trace_mask (smtp, MU_SMTP_TRACE_QRY, MU_XSCRIPT_PAYLOAD))
    _mu_smtp_xscript_level (smtp, MU_XSCRIPT_PAYLOAD);

  smtp->savebuf.type = MU_TRANSPORT_OUTPUT;
  if (mu_stream_ioctl (smtp->carrier, MU_IOCTL_TRANSPORT_BUFFER,
		       MU_IOCTL_OP_GET, &smtp->savebuf) == 0)
    {
      struct mu_buffer_query newbuf;
      newbuf.type = MU_TRANSPORT_OUTPUT;
      newbuf.buftype = mu_buffer_full;
      newbuf.bufsize = 64*1024;
      if (mu_stream_ioctl (smtp->carrier, MU_IOCTL_TRANSPORT_BUFFER,
			   MU_IOCTL_OP_SET, &newbuf) == 0)
	MU_SMTP_FSET (smtp, _MU_SMTP_SAVEBUF);
    }
  return 0;
}

int
_mu_smtp_data_end (mu_smtp_t smtp)
{
  int status = 0;
  /* code is always _MU_STR_EVENT_CLOSE */
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_SAVEBUF))
    {
      status = mu_stream_ioctl (smtp->carrier, MU_IOCTL_TRANSPORT_BUFFER,
				MU_IOCTL_OP_SET, &smtp->savebuf);
      if (status)
	mu_diag_output (MU_DIAG_NOTICE,
			"failed to restore buffer state on SMTP carrier: %s",
			mu_strerror (status));
    }
  _mu_smtp_xscript_level (smtp, MU_XSCRIPT_NORMAL);
  smtp->state = MU_SMTP_DOT;
  return status;
}

static void
_smtp_event_cb (struct _mu_stream *str, int code,
		unsigned long lval, void *pval)
{
  mu_smtp_t smtp = str->event_cb_data;
  _mu_smtp_data_end (smtp);
}

int
mu_smtp_data (mu_smtp_t smtp, mu_stream_t *pstream)
{
  int status;
  mu_stream_t input;

  if (!smtp)
    return EINVAL;
  if (MU_SMTP_FISSET (smtp, _MU_SMTP_ERR))
    return MU_ERR_FAILURE;
  if (smtp->state != MU_SMTP_MORE)
    return MU_ERR_SEQ;

  status = _mu_smtp_data_begin (smtp);
  if (status)
    return status;

  status = mu_filter_create (&input, smtp->carrier, "CRLFDOT",
			     MU_FILTER_ENCODE, MU_STREAM_WRITE);
  if (status)
    return status;
  input->event_cb = _smtp_event_cb;
  input->event_cb_data = smtp;
  input->event_mask = _MU_STR_EVMASK (_MU_STR_EVENT_CLOSE);
  *pstream = input;
  return 0;
}
