/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include <header.h>

int
header_set_value (header_t h, const char *fn, const char *fb, size_t n,
					 int replace)
{
  return h->_set_value (h, fn, fb, n, replace);
}

int
header_get_value (header_t h, const char *fn, char *fb,
		      size_t len, size_t *n)
{
  return h->_get_value (h, fn, fb, len, n);
}

int
header_get_mvalue (header_t h, const char *fn, char **fb, size_t *n)
{
  return h->_get_mvalue (h, fn, fb, n);
}
