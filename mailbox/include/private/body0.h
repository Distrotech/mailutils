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

#ifndef _BODY0_H
#define _BODY0_H

#include <io.h>
#include <body.h>

#include <stdio.h>
#include <sys/types.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /* __P */

#ifdef _cplusplus
extern "C" {
#endif

struct _body
{
  void *owner;

  /* it's better and more portable to use stdio */
  FILE *file;
  char *filename;
  stream_t stream;
  int (*_size)  (body_t, size_t*);
  int (*_lines) (body_t, size_t*);
};

#ifdef _cplusplus
}
#endif

#endif /* _BODY0_H */
