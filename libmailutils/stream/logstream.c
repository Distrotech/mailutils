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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/cctype.h>
#include <mailutils/log.h>

#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/logstream.h>

char *_mu_severity_str[] = {
  N_("DEBUG"),
  N_("INFO"),
  N_("NOTICE"),
  N_("WARNING"),
  N_("ERROR"),
  N_("CRIT"),
  N_("ALERT"),
  N_("EMERG"),
};
int _mu_severity_num = MU_ARRAY_SIZE (_mu_severity_str);

static int
_locus_set_file (struct mu_locus *loc, const char *file, size_t len)
{
  free (loc->mu_file);
  if (file)
    {
      loc->mu_file = malloc (len + 1);
      if (!loc->mu_file)
	return ENOMEM;
      memcpy (loc->mu_file, file, len);
      loc->mu_file[len] = 0;
    }
  else
    loc->mu_file = NULL;
  return 0;
}

#define _locus_set_line(loc, line) ((loc)->mu_line = line)
#define _locus_set_col(loc, col) ((loc)->mu_col = col)

static int
_log_write (struct _mu_stream *str, const char *buf, size_t size,
	    size_t *pnwrite)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  unsigned severity = sp->severity;
  int logmode = sp->logmode;
  struct mu_locus loc = sp->locus;
  const char *fname = NULL;
  unsigned flen = 0;
  int save_locus = 0;
  int rc;
  
#define NEXT do { buf++; size--; if (size == 0) return EINVAL; } while (0)
#define READNUM(n) do {				\
    unsigned __x = 0;				\
    if (*buf != '<')				\
      return EINVAL;				\
    NEXT;					\
    while (*buf != '>')				\
      {						\
        if (!mu_isdigit (*buf))			\
	  return EINVAL;			\
	__x = __x * 10 + (*buf - '0');		\
	NEXT;					\
      }						\
    NEXT;					\
    n = __x;					\
} while (0)

  /* Tell them we've consumed everything */
  *pnwrite = size;
  
  /* Process escapes */
  while (*buf == '\033')
    {
      int code;
      unsigned n;
      
      NEXT;
      code = *buf;
      NEXT;
      switch (code)
	{
	case 'S':
	  /* Save locus */
	  save_locus = 1;
	  break;

	case 's':
	  /* Severity code in decimal (single digit) */
	  if (!mu_isdigit (*buf))
	    return EINVAL;
	  severity = *buf - '0';
	  NEXT;
	  break;
	  
	case 'O':
	  /* Flags on.  Followed by a bitmask of MU_LOGMODE_*, in decimal  */
	  READNUM (n);
	  logmode |= n;
	  break;
	  
	case 'X':
	  /* Flags off.  Followed by a bitmask of MU_LOGMODE_*, in decimal  */
	  READNUM (n);
	  logmode &= ~n;
	  break;
	  
	case 'l':
	  /* Input line (decimal) */
	  READNUM (n);
	  _locus_set_line (&loc, n);
	  break;

	case 'c':
	  /* Column in input line (decimal) */
	  READNUM (n);
	  _locus_set_col (&loc, n);
	  break;
	  
	case 'f':
	  /* File name. Format: <N>S */
	  READNUM (flen);
	  fname = buf;
	  buf += flen;
	  size -= flen;
	  break;

	default:
	  return EINVAL;
	}
    }

  if (severity >= _mu_severity_num)
    severity = MU_LOG_EMERG;
  
  if (logmode & MU_LOGMODE_LOCUS)
    {
      if (fname)
	_locus_set_file (&loc, fname, flen);

      if (loc.mu_file)
	{
	  mu_stream_write (sp->transport, loc.mu_file,
			   strlen (loc.mu_file), NULL);
	  mu_stream_write (sp->transport, ":", 1, NULL);
	  if (loc.mu_line)
	    mu_stream_printf (sp->transport, "%u", loc.mu_line);
	  mu_stream_write (sp->transport, ":", 1, NULL);
	  if (loc.mu_col)
	    mu_stream_printf (sp->transport, "%u:", loc.mu_col);
	  mu_stream_write (sp->transport, " ", 1, NULL);
	}
    }

  if (save_locus)
    {
      _locus_set_file (&sp->locus, loc.mu_file, strlen (loc.mu_file));
      _locus_set_line (&sp->locus, loc.mu_line);
      _locus_set_col (&sp->locus, loc.mu_col);
    }
  
  if (fname)
    free (loc.mu_file);
  
  if (logmode & MU_LOGMODE_SEVERITY)
    {
      char *s = gettext (_mu_severity_str[severity]);
      rc = mu_stream_write (sp->transport, s, strlen (s), NULL);
      if (rc)
	return rc;
      mu_stream_write (sp->transport, ": ", 2, NULL);
    }
  return mu_stream_write (sp->transport, buf, size, NULL);
}

static int
_log_flush (struct _mu_stream *str)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  return mu_stream_flush (sp->transport);
}

static void
_log_done (struct _mu_stream *str)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  if (str->flags & MU_STREAM_AUTOCLOSE)
    mu_stream_destroy (&sp->transport);
}

static int
_log_close (struct _mu_stream *str)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  if (str->flags & MU_STREAM_AUTOCLOSE)
    return mu_stream_close (sp->transport);
  return 0;
}

static int
_log_setbuf_hook (mu_stream_t str, enum mu_buffer_type type, size_t size)
{
  if (type != mu_buffer_line)
    return EACCES;
  return 0;
}

static int
_log_ctl (struct _mu_stream *str, int op, void *arg)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  mu_transport_t *ptrans;
  
  switch (op)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!arg)
	return EINVAL;
      ptrans = arg;
      ptrans[0] = (mu_transport_t) sp->transport;
      ptrans[1] = NULL;
      break;

    case MU_IOCTL_SET_TRANSPORT:
      if (!arg)
	return EINVAL;
      ptrans = arg;
      if (ptrans[0])
	sp->transport = (mu_stream_t) ptrans[0];
      break;

    case MU_IOCTL_LOGSTREAM_GET_SEVERITY:
      if (!arg)
	return EINVAL;
      *(unsigned*)arg = sp->severity;
      break;
      
    case MU_IOCTL_LOGSTREAM_SET_SEVERITY:
      if (!arg)
	return EINVAL;
      if (*(unsigned*)arg >= _mu_severity_num)
	return EINVAL;
      sp->severity = *(unsigned*)arg;
      break;

    case MU_IOCTL_LOGSTREAM_GET_MODE:
      if (!arg)
	return EINVAL;
      *(int*)arg = sp->logmode;
      break;

    case MU_IOCTL_LOGSTREAM_SET_MODE:
      if (!arg)
	return EINVAL;
      sp->logmode = *(int*)arg;
      break;
      
    case MU_IOCTL_LOGSTREAM_GET_LOCUS:
      {
	struct mu_locus *ploc = arg;
	if (!arg)
	  return EINVAL;
	if (sp->locus.mu_file)
	  {
	    ploc->mu_file = strdup (sp->locus.mu_file);
	    if (!ploc->mu_file)
	      return ENOMEM;
	  }
	else
	  ploc->mu_file = NULL;
	ploc->mu_line = sp->locus.mu_line;
	ploc->mu_col = sp->locus.mu_col;
	break;
      }
	
    case MU_IOCTL_LOGSTREAM_SET_LOCUS:
      {
	struct mu_locus *ploc = arg;
	if (!arg)
	  {
	    free (sp->locus.mu_file);
	    sp->locus.mu_file = NULL;
	    sp->locus.mu_line = 0;
	    sp->locus.mu_col = 0;
	  }
	else
	  {
	    if (ploc->mu_file)
	      _locus_set_file (&sp->locus, ploc->mu_file,
			       strlen (ploc->mu_file));
	    if (ploc->mu_line)
	      _locus_set_line (&sp->locus, ploc->mu_line);
	    if (ploc->mu_col)
	      _locus_set_col (&sp->locus, ploc->mu_col);
	  }
	break;
      }
      
    default:
      return ENOSYS;
    }
  return 0;
}

int
mu_log_stream_create (mu_stream_t *pstr, mu_stream_t transport)
{
  struct _mu_log_stream *sp;
  mu_stream_t stream;
  int rc;
  
  sp = (struct _mu_log_stream *)
    _mu_stream_create (sizeof (*sp), MU_STREAM_WRITE);
  if (!sp)
    return ENOMEM;
  sp->base.write = _log_write;
  sp->base.flush = _log_flush;
  sp->base.close = _log_close;
  sp->base.done = _log_done;
  sp->base.setbuf_hook = _log_setbuf_hook;
  sp->base.ctl = _log_ctl;
  sp->transport = transport;
  mu_stream_ref (transport);
  sp->severity = MU_LOG_ERROR;
  sp->logmode = 0;
  
  stream = (mu_stream_t) sp;
  mu_stream_set_buffer (stream, mu_buffer_line, 0);
  rc = mu_stream_open (stream);
  if (rc)
    mu_stream_destroy (&stream);
  else
    *pstr = stream;

  return rc;
}


