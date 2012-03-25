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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>
#include <mailutils/log.h>

#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/debug.h>
#include <mailutils/sys/logstream.h>

char *_mu_severity_str[] = {
  N_("debug"),
  N_("info"),
  N_("notice"),
  N_("warning"),
  N_("error"),
  N_("crit"),
  N_("alert"),
  N_("emerg"),
};
int _mu_severity_num = MU_ARRAY_SIZE (_mu_severity_str);

int
mu_severity_from_string (const char *str, unsigned *pn)
{
  int i;

  for (i = 0; i < _mu_severity_num; i++)
    {
      if (mu_c_strcasecmp (_mu_severity_str[i], str) == 0)
	{
	  *pn = i;
	  return 0;
	}
    }
  return MU_ERR_NOENT;
}

int
mu_severity_to_string (unsigned n, const char **pstr)
{
  if (n >= _mu_severity_num)
    return MU_ERR_NOENT;
  *pstr = _mu_severity_str[n];
  return 0;
}

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
  int escape_error = 0;
  
#define NEXT do { if (size == 0) return EINVAL; buf++; size--; } while (0)
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
  while (!escape_error && *buf == '\033')
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
	  if (*buf == '<')
	    READNUM (severity);
	  else if (mu_isdigit (*buf))
	    {
	      severity = *buf - '0';
	      NEXT;
	    }
	  else
	    return EINVAL;
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
	  buf -= 2;
	  size += 2;
	  escape_error = 1;
	}
    }

  if (severity >= _mu_severity_num)
    severity = MU_LOG_EMERG;

  if (fname)
    {
      loc.mu_file = NULL;
      _locus_set_file (&loc, fname, flen);
    }
  
  if (save_locus)
    {
      _locus_set_file (&sp->locus, loc.mu_file, strlen (loc.mu_file));
      _locus_set_line (&sp->locus, loc.mu_line);
      _locus_set_col (&sp->locus, loc.mu_col);
    }
  
  if (severity < sp->threshold)
    {
      if (fname)
	free (loc.mu_file);
      return 0;
    }
  
  mu_stream_ioctl (sp->transport, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_SET_SEVERITY, &severity);
  
  if (logmode & MU_LOGMODE_LOCUS)
    {
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

  if (fname)
    free (loc.mu_file);
  
  if ((logmode & MU_LOGMODE_SEVERITY) &&
      !(sp->sevmask & MU_DEBUG_LEVEL_MASK(severity)))
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
  mu_stream_destroy (&sp->transport);
}

static int
_log_close (struct _mu_stream *str)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  return mu_stream_close (sp->transport);
}

static int
_log_setbuf_hook (mu_stream_t str, enum mu_buffer_type type, size_t size)
{
  if (type != mu_buffer_line)
    return EACCES;
  return 0;
}

static int
_log_ctl (struct _mu_stream *str, int code, int opcode, void *arg)
{
  struct _mu_log_stream *sp = (struct _mu_log_stream *)str;
  int status;
  
  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) sp->transport;
	      ptrans[1] = NULL;
	      break;

	    case MU_IOCTL_OP_SET:
	      ptrans = arg;
	      if (ptrans[0])
		sp->transport = (mu_stream_t) ptrans[0];
	      break;

	    default:
	      return EINVAL;
	    }
	}
      break;

    case MU_IOCTL_SUBSTREAM:
      if (sp->transport &&
          ((status = mu_stream_ioctl (sp->transport, code, opcode, arg)) == 0 ||
           status != ENOSYS))
        return status;
      /* fall through */

    case MU_IOCTL_TOPSTREAM:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_stream_t *pstr = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      pstr[0] = sp->transport;
	      mu_stream_ref (pstr[0]);
	      pstr[1] = NULL;
	      break;

	    case MU_IOCTL_OP_SET:
	      mu_stream_unref (sp->transport);
	      sp->transport = pstr[0];
	      mu_stream_ref (sp->transport);
	      break;

	    default:
	      return EINVAL;
	    }
	}
      break;
      
    case MU_IOCTL_LOGSTREAM:
      switch (opcode)
	{
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
	  if (!arg)
	    return EINVAL;
	  else
	    {
	      struct mu_locus *ploc = arg;
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
	    }
	  break;
	
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

	case MU_IOCTL_LOGSTREAM_SET_LOCUS_LINE:
	  if (!arg)
	    return EINVAL;
	  sp->locus.mu_line = *(unsigned*)arg;
	  break;
	  
	case MU_IOCTL_LOGSTREAM_SET_LOCUS_COL:
	  if (!arg)
	    return EINVAL;
	  sp->locus.mu_col = *(unsigned*)arg;
	  break;
	  
	case MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE:
	  if (!arg)
	    sp->locus.mu_line++;
	  else
	    sp->locus.mu_line += *(int*)arg;
	  break;

	case MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_COL:
	  if (!arg)
	    sp->locus.mu_col++;
	  else
	    sp->locus.mu_col += *(int*)arg;
	  break;

	case MU_IOCTL_LOGSTREAM_SUPPRESS_SEVERITY:
	  if (!arg)
	    sp->threshold = MU_LOG_DEBUG;
	  else if (*(unsigned*)arg >= _mu_severity_num)
	    return MU_ERR_NOENT;
	  sp->threshold = *(unsigned*)arg;
	  break;
	  
	case MU_IOCTL_LOGSTREAM_SUPPRESS_SEVERITY_NAME:
	  if (!arg)
	    sp->threshold = MU_LOG_DEBUG;
	  else
	    return mu_severity_from_string ((const char *) arg, &sp->threshold);

	case MU_IOCTL_LOGSTREAM_GET_SEVERITY_MASK:
	  if (!arg)
	    return EINVAL;
	  *(int*)arg = sp->sevmask;
	  break;

	case MU_IOCTL_LOGSTREAM_SET_SEVERITY_MASK:
	  if (!arg)
	    return EINVAL;
	  sp->sevmask = *(int*)arg;
	  break;

	case MU_IOCTL_LOGSTREAM_CLONE:
	  if (!arg)
	    return EINVAL;
	  else
	    {
	      mu_stream_t str;
	      struct _mu_log_stream *newp;
	      int rc = mu_log_stream_create (&str, sp->transport);
	      if (rc)
		return rc;
	      newp = (struct _mu_log_stream *) str;
	      newp->severity = sp->severity;
	      newp->threshold = sp->threshold;
	      newp->logmode = sp->logmode;
	      newp->sevmask = sp->sevmask;
	      if (sp->locus.mu_file)
                {
		  newp->locus.mu_file = strdup (sp->locus.mu_file);
		  if (!newp->locus.mu_file)
		    return ENOMEM;
		}
	      newp->locus.mu_line = sp->locus.mu_line;
	      newp->locus.mu_col = sp->locus.mu_col;
	      *(mu_stream_t*) arg = str;
	    }
	  break;
	  
	default:
	  return EINVAL;
	}
      break;

    case MU_IOCTL_FILTER:
    case MU_IOCTL_SYSLOGSTREAM:
      return mu_stream_ioctl (sp->transport, code, opcode, arg);
            
    default:
      return ENOSYS;
    }
  return 0;
}

void
_mu_log_stream_setup (struct _mu_log_stream *sp, mu_stream_t transport)
{
  mu_stream_t stream;
  
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
}

int
mu_log_stream_create (mu_stream_t *pstr, mu_stream_t transport)
{
  struct _mu_log_stream *sp;
  
  sp = (struct _mu_log_stream *)
    _mu_stream_create (sizeof (*sp), MU_STREAM_WRITE);
  if (!sp)
    return ENOMEM;
  _mu_log_stream_setup (sp, transport);
  *pstr = (mu_stream_t) sp;
  
  return 0;
}


