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

#ifndef _IO0_H
# define _IO0_H

#include <io.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

struct _stream
{
  void *owner;
  int flags;
  void (*_destroy) __P ((void *));
  int (*_get_fd) __P ((stream_t, int *));
  int (*_read) __P ((stream_t, char *, size_t, off_t, size_t *));
  int (*_write) __P ((stream_t, const char *, size_t, off_t, size_t *));
};

#ifdef __cplusplus
}
#endif

#endif /* _IO0_H */
