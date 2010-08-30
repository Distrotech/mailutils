/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 
   2005, 2006, 2007, 2008, 2009, 2010 Free Software Foundation, Inc.

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
#include <mailutils/alloc.h>
#include <mailutils/errno.h>

#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/dbgstream.h>
#include <mailutils/debug.h>

static int
_dbg_write (struct _mu_stream *str, const char *buf, size_t size,
	    size_t *pnwrite)
{
  struct _mu_dbgstream *sp = (struct _mu_dbgstream *)str;
  if (pnwrite)
    *pnwrite = size;
  while (size > 0 && (buf[size-1] == '\n' || buf[size-1] == '\r'))
    size--;
  if (size)
    mu_debug_printf (sp->debug, sp->level, "%.*s\n", size, buf);
  return 0;
}

static int
_dbg_flush (struct _mu_stream *str)
{
  struct _mu_dbgstream *sp = (struct _mu_dbgstream *)str;
  mu_debug_printf (sp->debug, sp->level, "\n");
  return 0;
}

static void
_dbg_done (struct _mu_stream *str)
{
  struct _mu_dbgstream *sp = (struct _mu_dbgstream *)str;
  if (str->flags & MU_STREAM_AUTOCLOSE)
    mu_debug_destroy (&sp->debug, NULL);
}

int
mu_dbgstream_create(mu_stream_t *pref, mu_debug_t debug, mu_log_level_t level,
		    int flags)
{
  struct _mu_dbgstream *sp;

  sp = (struct _mu_dbgstream *)
    _mu_stream_create (sizeof (*sp), MU_STREAM_WRITE |
		       (flags & MU_STREAM_AUTOCLOSE));
  if (!sp)
    return ENOMEM;
  sp->stream.write = _dbg_write;
  sp->stream.flush = _dbg_flush;
  sp->stream.done = _dbg_done;
  
  sp->debug = debug;
  sp->level = level;
  mu_stream_set_buffer ((mu_stream_t) sp, mu_buffer_line, 1024);
  *pref = (mu_stream_t) sp;
  return 0;
}
  
