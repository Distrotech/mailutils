/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

/* Notes:

 */

#ifndef _FILTER0_H
#define _FILTER0_H

#include <mailutils/filter.h>
#include <mailutils/list.h>
#include <mailutils/monitor.h>
#include <mailutils/property.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

#ifdef __cplusplus
extern "C" {
#endif

struct _filter
{
  stream_t stream;
  stream_t filter_stream;
  property_t property;
  int direction;
  int type;
  void *data;
  int  (*_read)     __P ((filter_t, char *, size_t, off_t, size_t *));
  int  (*_readline) __P ((filter_t, char *, size_t, off_t, size_t *));
  int  (*_write)    __P ((filter_t, const char *, size_t, off_t, size_t *));
  void (*_destroy)  __P ((filter_t));
};

#ifdef __cplusplus
}
#endif

#endif /* _FILTER0_H */
