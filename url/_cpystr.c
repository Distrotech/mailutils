/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include <cpystr.h>
#include <string.h>

int
_cpystr (const char *src, char *dst, unsigned int size)
{
  unsigned int len = src ? strlen (src) : 0 ;

  if (dst == NULL || size == 0)
    {
      return len;
    }

  if (len >= size)
    {
      len = size - 1;
    }
  memcpy (dst, src, len);
  dst[len] = '\0';
  return len;
}
