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
#include <mailutils/mutil.h>


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
  char *fname;
  struct _mu_file_stream *str;

  if (!dir)
    fname = NULL;
  else if ((fname = mu_strdup (dir)) == NULL)
    return ENOMEM;
  
  rc = _mu_file_stream_create (pstream,
			       sizeof (struct _mu_file_stream),
			       fname, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (rc)
    {
      free (fname);
      return rc;
    }
  str = (struct _mu_file_stream *) *pstream;

  str->stream.open = fd_temp_open;
  str->flags = _MU_FILE_STREAM_TEMP;

  return 0;
}
