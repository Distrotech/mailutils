/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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
  syslog (lsp->prio, "%*.*s", (int) size, (int) size, buf);
  return 0;
}

static int
_syslog_ctl (struct _mu_stream *str, int op, void *arg)
{
  struct _mu_syslog_stream *sp = (struct _mu_syslog_stream *)str;

  switch (op)
    {
    case MU_IOCTL_LOGSTREAM_GET_SEVERITY:
      if (!arg)
	return EINVAL;
      *(int*)arg = sp->prio;
      break;
      
    case MU_IOCTL_LOGSTREAM_SET_SEVERITY:
      if (!arg)
	return EINVAL;
      sp->prio = *(int*)arg;
      break;
      
    default:
      return ENOSYS;
    }
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
  str->base.write = _syslog_stream_write;
  str->base.ctl = _syslog_ctl;
  str->base.setbuf_hook = _syslog_setbuf_hook;
  *pstream = (mu_stream_t) str;
  mu_stream_set_buffer (*pstream, mu_buffer_line, 0);
  return 0;
}
