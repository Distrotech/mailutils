/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

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
#include <mailutils/sys/file_stream.h>
#include <mailutils/util.h>


static int
fd_temp_open (struct _mu_stream *str)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  int fd = mu_tempfile (fstr->filename, NULL);
  if (fd == -1)
    return errno;
  fstr->fd = fd;
  return 0;
}
  
int
mu_temp_file_stream_create (mu_stream_t *pstream, const char *dir)
{
  int rc;
  struct _mu_file_stream *str;
  mu_stream_t stream;
  rc = _mu_file_stream_create (&str,
			       sizeof (struct _mu_file_stream),
			       dir,
			       -1,
			       MU_STREAM_RDWR | MU_STREAM_SEEK |
			       MU_STREAM_CREAT | 
			       MU_STREAM_AUTOCLOSE);
  if (rc == 0)
    {
      str->stream.open = fd_temp_open;
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
