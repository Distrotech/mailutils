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


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <header0.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int
header_init (header_t *ph, const char *blurb, size_t len, int flag)
{
  if (flag != MU_HEADER_RFC822)
    return ENOTSUP;
  return rfc822_init (ph, blurb, len);
}

void
header_destroy (header_t *ph)
{
  if (ph && *ph)
    (*ph)->_destroy (ph);
}

/* stub functions */
int
header_set_value (header_t h, const char *fn, const char *fb, size_t n,
					 int replace)
{
  if (h == NULL || h->_set_value == NULL)
    return EINVAL;
  return h->_set_value (h, fn, fb, n, replace);
}

int
header_get_value (header_t h, const char *fn, char *fb,
		      size_t len, size_t *n)
{
  if (h == NULL || h->_get_value == NULL )
    return EINVAL;
  return h->_get_value (h, fn, fb, len, n);
}

int
header_entry_name (header_t h, size_t num, char *buf, size_t len, size_t *n)
{
  if (h == NULL || h->_entry_name == NULL)
    return EINVAL;

  return h->_entry_name (h, num, buf, len, n);
}

int
header_entry_value (header_t h, size_t num, char *buf, size_t len, size_t *n)
{
  if (h == NULL || h->_entry_value == NULL)
    return EINVAL;

  return h->_entry_value (h, num, buf, len, n);
}

int
header_entry_count (header_t h, size_t *num)
{
  if (h == NULL || h->_entry_count)
    return EINVAL;
  return h->_entry_count (h, num);
}

ssize_t
header_get_data (header_t h, char *data, size_t len, off_t off, int *err)
{
  if (h == NULL || h->_get_data)
    return EINVAL;
  return h->_get_data (h, data, len, off, err);
}
