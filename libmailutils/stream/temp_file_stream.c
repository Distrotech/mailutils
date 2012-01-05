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

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/temp_file_stream.h>
#include <mailutils/util.h>


static int
fd_temp_open (struct _mu_stream *str)
{
  struct _mu_temp_file_stream *fstr = (struct _mu_temp_file_stream *) str;
  return mu_tempfile (&fstr->hints, fstr->hflags, &fstr->stream.fd, NULL);
}

static void
fd_temp_done (struct _mu_stream *str)
{
  struct _mu_temp_file_stream *fstr = (struct _mu_temp_file_stream *) str;
  if (fstr->hflags & MU_TEMPFILE_TMPDIR)
    free (fstr->hints.tmpdir);
  if (fstr->hflags & MU_TEMPFILE_SUFFIX)
    free (fstr->hints.suffix);
  if (fstr->file_done)
    fstr->file_done (&fstr->stream.stream);
}

int
mu_temp_file_stream_create (mu_stream_t *pstream,
			    struct mu_tempfile_hints *hints, int flags)
{
  int rc;
  struct _mu_file_stream *str;
  mu_stream_t stream;

  if (flags && !hints)
    return EINVAL;
  rc = _mu_file_stream_create (&str,
			       sizeof (struct _mu_temp_file_stream),
			       NULL,
			       -1,
			       MU_STREAM_RDWR | MU_STREAM_SEEK |
			       MU_STREAM_CREAT);
  if (rc == 0)
    {
      struct _mu_temp_file_stream *tstr = (struct _mu_temp_file_stream *)str;
      tstr->stream.stream.open = fd_temp_open;
      tstr->file_done = tstr->stream.stream.done;
      tstr->stream.stream.done = fd_temp_done;

      if ((flags & MU_TEMPFILE_TMPDIR) &&
	  (tstr->hints.tmpdir = strdup (hints->tmpdir)) == NULL)
	{
	  mu_stream_unref ((mu_stream_t) str);
	  return ENOMEM;
	}
      if ((flags & MU_TEMPFILE_SUFFIX) &&
	  (tstr->hints.suffix = strdup (hints->suffix)) == NULL)
	{
	  mu_stream_unref ((mu_stream_t) str);
	  return ENOMEM;
	}
      tstr->hflags = flags & ~MU_TEMPFILE_MKDIR;
      
      str->flags = _MU_FILE_STREAM_TEMP;
      stream = (mu_stream_t) str;
      rc = mu_stream_open (stream);
      if (rc)
	mu_stream_unref (stream);
      else
	{
	  mu_stream_set_buffer (stream, mu_buffer_full, 0);
	  *pstream = stream;
	}
    }
  return 0;
}
