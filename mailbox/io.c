/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <io0.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int
istream_init (istream_t *pis, int (*_read)
	      __P ((istream_t, char *, size_t, off_t, size_t *)),
	      void *owner)
{
  istream_t is;
  if (pis == NULL)
    return EINVAL;
  is = calloc (1, sizeof (*is));
  if (is == NULL)
    return ENOMEM;
  is->owner = owner;
  is->_read = _read;
  *pis = is;
  return 0;
}

int
ostream_init (ostream_t *pos, int (*_write)
		   __P ((ostream_t, const char *, size_t, off_t, size_t *)),
	      void *owner)
{
  ostream_t os;
  if (pos == NULL)
    return EINVAL;
  os = calloc (1, sizeof (*os));
  if (os == NULL)
    return ENOMEM;
  os->owner = owner;
  os->_write = _write;
  *pos = os;
  return 0;
}

void
istream_destroy (istream_t *pis, void *owner)
{
  if (pis && *pis)
    {
      istream_t is = *pis;
      is->ref_count--;
      if ((is->owner && is->owner == owner) ||
	  (is->owner == NULL && is->ref_count <= 0))
	free (is);
      *pis = NULL;
    }
}

void
ostream_destroy (ostream_t *pos, void *owner)
{
  if (pos && (*pos))
    {
      ostream_t os = *pos;
      os->ref_count--;
      if ((os->owner && os->owner == owner) ||
	  (os->owner == NULL && os->ref_count <= 0))
	free (os);
      *pos = NULL;
    }
}

int
istream_read (istream_t is, char *buf, size_t count,
	      off_t offset, size_t *pnread)
{
  if (is == NULL || is->_read == NULL)
    return EINVAL;
  return is->_read (is, buf, count, offset, pnread);
}

int
ostream_write (ostream_t os, const char *buf, size_t count,
	       off_t offset, size_t *pnwrite)
{
  if (os == NULL || os->_write == NULL)
      return EINVAL;
  return os->_write (os, buf, count, offset, pnwrite);
}
