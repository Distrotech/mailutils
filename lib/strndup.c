/* Copyright (C) 1996, 1997, 1998, 2001 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

#if defined _LIBC || defined STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
char *malloc ();
#endif

char *
strndup (s, n)
     const char *s;
     size_t n;
{
  size_t len = strnlen (s, n);
  char *nouveau = malloc (len + 1);

  if (nouveau == NULL)
    return NULL;

  nouveau[len] = '\0';
  return (char *) memcpy (nouveau, s, len);
}
