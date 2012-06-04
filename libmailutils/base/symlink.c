/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2003, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <mailutils/types.h>
#include <mailutils/util.h>

#define INITIAL_READLINK_SIZE 128

int
mu_readlink (const char *name, char **pbuf, size_t *psize, size_t *plen)
{
  size_t status = 0;
  char *buf = *pbuf;
  size_t size = *psize;
  ssize_t linklen;

  while (1)
    {
      if (!buf)
	{
	  size = INITIAL_READLINK_SIZE;
	  buf = malloc (size);
	}
      else
	{
	  char *p;
	  size_t newsize = size << 1;
	  if (newsize < size)
	    {
	      status = ENAMETOOLONG;
	      break;
	    }
	  size = newsize;
	  p = realloc (buf, size);
	  if (!p)
	    free (buf);
	  buf = p;
	}
      if (!buf)
	{
	  status = ENOMEM;
	  break;
	}
      
      linklen = readlink (name, buf, size);
      if (linklen < 0 && errno != ERANGE)
	{
	  status = errno;
	  break;
	}

      if ((size_t) linklen < size)
	{
	  buf[linklen++] = '\0';
	  status = 0;
	  break;
	}
    }

  if (status)
    {
      if (buf)
	{
	  free (buf);
	  buf = NULL;
	}
      size = 0;
    }
  *pbuf = buf;
  *psize = size;
  if (plen)
    *plen = linklen;
  return status;
}

#ifndef _POSIX_SYMLOOP_MAX
# define _POSIX_SYMLOOP_MAX 255
#endif

int
mu_unroll_symlink (const char *name, char **pout)
{
  size_t symloops = 0;
  struct slbuf { char *base; size_t size; } buf[2];
  int idx = 0;
  int status;
  
  buf[0].base = buf[1].base = NULL;
  buf[0].size = buf[1].size = 0;

  while (symloops++ < _POSIX_SYMLOOP_MAX)
    {
      struct stat st;
      size_t len;

      if (lstat (name, &st) == -1)
	return errno;

      if (!S_ISLNK (st.st_mode))
	{
	  if (!buf[idx].base)
	    {
	      buf[idx].base = strdup (name);
	      if (!buf[idx].base)
		return ENOMEM;
	    }
	  status = 0;
	  break;
	}

      idx = !idx;
      status = mu_readlink (name, &buf[idx].base, &buf[idx].size, &len);
      if (status)
	break;
      name = mu_normalize_path (buf[idx].base);
    }

  if (status)
    {
      free (buf[0].base);
      free (buf[1].base);
    }
  else
    {
      *pout = buf[idx].base;
      free (buf[!idx].base);
    }
  return status;
}
