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

struct _istream
{
  /* owner of the stream can not be a specific type */
  void *owner;
  int ref_count;
  int (*_read) __P ((istream_t, char *, size_t, off_t, size_t *));
};

struct _ostream
{
  /* owner of the stream can not be a specific type */
  void *owner;
  int ref_count;
  int (*_write) __P ((ostream_t, const char *, size_t, off_t, size_t *));
};

extern int istream_init __P ((istream_t *,
			      int (*_read) __P ((istream_t, char *,
						 size_t, off_t, size_t *)),
			      void *owner));
extern void istream_destroy __P ((istream_t *, void *owner));
extern int ostream_init __P ((ostream_t *,
			      int (*_write) __P ((ostream_t, const char *,
						  size_t, off_t, size_t *)),
			      void *owner));
extern void ostream_destroy __P ((ostream_t *, void *owner));

#ifdef __cplusplus
}
#endif

#endif /* _IO0_H */
