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
istream_init (istream_t *pis)
{
  istream_t is;
  if (pis == NULL)
    return EINVAL;
  is = calloc (1, sizeof (*is));
  if (is == NULL)
    return ENOMEM;
  *pis = is;
  return 0;
}

int
ostream_init (ostream_t *pos)
{
  ostream_t os;
  if (pos == NULL)
    return EINVAL;
  os = calloc (1, sizeof (*os));
  if (os == NULL)
    return ENOMEM;
  *pos = os;
  return 0;
}

void
istream_destroy (istream_t *pis)
{
  free (pis);
}

void
ostream_destroy (ostream_t *pos)
{
  free (pos);
}

int
istream_set_read (istream_t is, ssize_t (*read)
		  __P ((istream_t, char *, size_t, off_t)))
{
  if (is == NULL)
    return EINVAL;
  is->_read = read;
  return 0;
}

int
ostream_set_write (ostream_t os, ssize_t (*write)
		   __P ((ostream_t, const char *, size_t, off_t)))
{
  if (os == NULL)
    return EINVAL;
  os->_write = write;
  return 0;
}

ssize_t
istream_read (istream_t is, char *buf, size_t count, off_t offset)
{
  if (is == NULL || is->_read == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  return is->_read (is, buf, count, offset);
}

ssize_t
ostream_write (ostream_t os, const char *buf, size_t count, off_t offset)
{
  if (os == NULL || os->_write == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  return os->_write (os, buf, count, offset);
}
