/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/file_stream.h>

int
mu_stdio_stream_create (mu_stream_t *pstream, int fd, int flags)
{
  int rc;
  char *filename;
  mu_stream_t transport;
  int need_cache;
  struct _mu_file_stream *fstr;
  
  switch (fd)
    {
    case MU_STDIN_FD:
      flags |= MU_STREAM_READ;
      break;
      
    case MU_STDOUT_FD:
    case MU_STDERR_FD:
      flags |= MU_STREAM_WRITE;
    }

  need_cache = flags & MU_STREAM_SEEK;
  if (need_cache && (flags & MU_STREAM_WRITE))
    /* Write caches are not supported */
    return EINVAL;

  if (flags & MU_STREAM_READ)
    filename = "<stdin>";
  else
    filename = "<stdout>";

  /* Create transport stream. */
  rc = _mu_file_stream_create (&fstr, sizeof (*fstr),
			       filename, fd, flags & ~MU_STREAM_SEEK);
  if (rc)
    return rc;
  fstr->stream.flags |= _MU_STR_OPEN;
  fstr->stream.open = NULL;
  transport = (mu_stream_t) fstr;
  mu_stream_set_buffer (transport, mu_buffer_line, 0);
  
  /* Wrap it in cache, if required */
  if (need_cache)
    {
      mu_stream_t str;
      rc = mu_rdcache_stream_create (&str, transport, flags);
      mu_stream_unref (transport);
      if (rc)
	return rc;
      transport = str;
    }

  *pstream = transport;
  return 0;
}

