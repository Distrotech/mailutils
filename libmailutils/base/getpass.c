/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2012 Free Software Foundation, Inc.

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

#include <mailutils/types.h>
#include <mailutils/stream.h>
#include <mailutils/util.h>
#include <mailutils/cstr.h>

int
mu_getpass (mu_stream_t in, mu_stream_t out, const char *prompt,
	    char **passptr)
{
  int status;
  int echo_state = 0;
  size_t size = 0;
  char *buf = NULL;

  status = mu_stream_write (out, prompt, strlen (prompt), NULL);
  if (status)
    return status;
  mu_stream_flush (out);
  status = mu_stream_ioctl (in, MU_IOCTL_ECHO, MU_IOCTL_OP_SET, &echo_state);
  if (status == 0)
    echo_state = 1;
  status = mu_stream_getline (in, &buf, &size, NULL);
  if (echo_state)
    {
      mu_stream_ioctl (in, MU_IOCTL_ECHO, MU_IOCTL_OP_SET, &echo_state);
      mu_stream_write (out, "\n", 1, NULL);
    }
  if (status == 0)
    {
      mu_rtrim_cset (buf, "\n");
      *passptr = buf;
    }
  return 0;
}

