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
#include <errno.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/sys/nullstream.h>
#include <mailutils/stream.h>
#include <mailutils/cctype.h>

static int
_nullstream_read (struct _mu_stream *str, char *buf, size_t bufsize,
		  size_t *pnread)
{
  struct _mu_nullstream *np = (struct _mu_nullstream *)str;
  size_t i;
  mu_off_t off;
  
  if (np->pattern == NULL)
    {
      *pnread = 0;
      return 0;
    }

  off = np->base.offset + np->base.pos;
  for (i = 0; i < bufsize; i++, off++)
    {
      if ((np->mode & MU_NULLSTREAM_SIZE) && off >= np->size)
	break;
      *buf++ = np->pattern[off % np->patsize];
    }
  *pnread = i;
  return 0;
}

static int
_nullstream_write (struct _mu_stream *str, const char *buf, size_t bufsize,
		   size_t *pnwrite)
{
  *pnwrite = bufsize;
  return 0;
}

static void
_nullstream_free_pattern (struct _mu_nullstream *np)
{
  if (!(np->mode & MU_NULLSTREAM_PATSTAT))
    {
      free (np->pattern);
      np->pattern = NULL;
      np->patsize = 0;
      np->mode &= ~MU_NULLSTREAM_PATSTAT;
    }
}

static void
_nullstream_done (struct _mu_stream *str)
{
  struct _mu_nullstream *np = (struct _mu_nullstream *)str;
  _nullstream_free_pattern (np);
}

static int
_nullstream_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *ppos)
{
  struct _mu_nullstream *np = (struct _mu_nullstream *)str;
  if ((np->mode & MU_NULLSTREAM_SIZE) && off >= np->size)
    return ESPIPE;
  *ppos = off;
  return 0;
}

static int
_nullstream_size (struct _mu_stream *str, mu_off_t *psize)
{
  struct _mu_nullstream *np = (struct _mu_nullstream *)str;
  *psize = (np->mode & MU_NULLSTREAM_SIZE) ? np->size : 0;
  return 0;
}

static int
_nullstream_truncate (struct _mu_stream *str, mu_off_t size)
{
  struct _mu_nullstream *np = (struct _mu_nullstream *)str;
  np->base.size = _nullstream_size;
  np->size = size;
  np->mode |= MU_NULLSTREAM_SIZE;
  return 0;
}

static int
_nullstream_ctl (struct _mu_stream *str, int code, int opcode, void *arg)
{
  struct _mu_nullstream *np = (struct _mu_nullstream *)str;

  if (code != MU_IOCTL_NULLSTREAM)
    /* Only this code is supported */
    return ENOSYS;
  switch (opcode)
    {
    case MU_IOCTL_NULLSTREAM_SET_PATTERN:
      if (!arg)
	_nullstream_free_pattern (np);
      else
	{
	  struct mu_nullstream_pattern *pat = arg;
	  char *p;
	  
	  p = malloc (pat->size);
	  if (!p)
	    return ENOMEM;
	  memcpy (p, pat->pattern, pat->size);
	  _nullstream_free_pattern (np);
	  np->pattern = p;
	  np->patsize = pat->size;
	}
      break;

    case MU_IOCTL_NULLSTREAM_SET_PATCLASS:
      if (!arg)
	return EINVAL;
      else
	{
	  char buf[256];
	  int cnt = 0, i;
	  int class = *(int*)arg;
	  char *p;
	  
	  for (i = 0; i < 256; i++)
	    {
	      if (mu_c_is_class (i, class))
		buf[cnt++] = i;
	    }

	  p = malloc (cnt);
	  if (!p)
	    return ENOMEM;
	  memcpy (p, buf, cnt);
	  _nullstream_free_pattern (np);
	  np->pattern = p;
	  np->patsize = cnt;
	}
      break;
	  
    case MU_IOCTL_NULLSTREAM_SETSIZE:
      if (!arg)
	return EINVAL;
      else
	return _nullstream_truncate (str, *(mu_off_t*)arg);
      break;

    case MU_IOCTL_NULLSTREAM_CLRSIZE:
      np->mode &= ~MU_NULLSTREAM_SIZE;
      np->base.size = NULL;
      break;
      
    default:
      return ENOSYS;
    }
  return 0;
}

int
mu_nullstream_create (mu_stream_t *pref, int flags)
{
  struct _mu_nullstream *np;

  np = (struct _mu_nullstream *)
         _mu_stream_create (sizeof (*np),
			    flags | MU_STREAM_SEEK | _MU_STR_OPEN);
  if (!np)
    return ENOMEM;
  np->base.read = _nullstream_read; 
  np->base.write = _nullstream_write;
  np->base.seek = _nullstream_seek; 
  np->base.ctl = _nullstream_ctl;
  np->base.truncate = _nullstream_truncate;
  np->base.done = _nullstream_done;

  np->pattern = "";
  np->patsize = 0;
  np->mode = MU_NULLSTREAM_PATSTAT;
  
  *pref = (mu_stream_t) np;

  mu_stream_set_buffer (*pref, mu_buffer_full, 0);
  return 0;
}
