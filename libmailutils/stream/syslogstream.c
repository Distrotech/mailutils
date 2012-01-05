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

#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/sys/syslogstream.h>

static int
_syslog_stream_write (struct _mu_stream *stream, const char *buf,
		      size_t size, size_t *pret)
{
  struct _mu_syslog_stream *lsp = (struct _mu_syslog_stream *)stream;

  *pret = size;
  if (size > 0 && buf[size-1] == '\n')
    size--;
  if (size == 0)
    return 0;
  if (lsp->logger)
    lsp->logger (lsp->prio, "%*.*s", (int) size, (int) size, buf);
  else
    syslog (lsp->prio, "%*.*s", (int) size, (int) size, buf);
  return 0;
}

static int sev2prio[] = {
  LOG_DEBUG,
  LOG_INFO,
  LOG_NOTICE,
  LOG_WARNING,
  LOG_ERR,
  LOG_CRIT,
  LOG_ALERT,
  LOG_EMERG
};

static int
_syslog_ctl (struct _mu_stream *str, int code, int opcode, void *arg)
{
  struct _mu_syslog_stream *sp = (struct _mu_syslog_stream *)str;
  unsigned n;

  if (code == MU_IOCTL_LOGSTREAM)
    switch (opcode)
      {
      case MU_IOCTL_LOGSTREAM_GET_SEVERITY:
	if (!arg)
	  return EINVAL;
	for (n = 0; n < MU_ARRAY_SIZE (sev2prio); n++)
	  if (sev2prio[n] == sp->prio)
	    {
	      *(int*)arg = n;
	      break;
	    }
	return MU_ERR_FAILURE;
	
      case MU_IOCTL_LOGSTREAM_SET_SEVERITY:
	if (!arg)
	  return EINVAL;
	n = *(unsigned*)arg;
	if (n < MU_ARRAY_SIZE (sev2prio))
	  sp->prio = sev2prio[n];
	else
	  return EINVAL;
	break;

      default:
	return ENOSYS;
      }
  else if (code == MU_IOCTL_SYSLOGSTREAM)
    {
      if (!arg)
	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_SYSLOGSTREAM_SET_LOGGER:
	  sp->logger = arg;
	  break;

	case MU_IOCTL_SYSLOGSTREAM_GET_LOGGER:
	  *(void (**) (int, const char *, ...))arg = sp->logger;
	  break;

	default:
	  return ENOSYS;
	}
    }
  else
    return ENOSYS;
  return 0;
}

int
_syslog_setbuf_hook (mu_stream_t str, enum mu_buffer_type type, size_t size)
{
  if (type != mu_buffer_line)
    return EACCES;
  return 0;
}

int
mu_syslog_stream_create (mu_stream_t *pstream, int prio)
{
  struct _mu_syslog_stream *str =
    (struct _mu_syslog_stream *) _mu_stream_create (sizeof (*str),
						    MU_STREAM_WRITE);
  if (!str)
    return ENOMEM;
  str->prio = prio;
  str->logger = NULL;
  str->base.write = _syslog_stream_write;
  str->base.ctl = _syslog_ctl;
  str->base.setbuf_hook = _syslog_setbuf_hook;
  *pstream = (mu_stream_t) str;
  mu_stream_set_buffer (*pstream, mu_buffer_line, 0);
  return 0;
}
