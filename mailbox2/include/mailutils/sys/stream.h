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

#ifndef _MAILUTILS_SYS_STREAM_H
#define _MAILUTILS_SYS_STREAM_H

#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _stream_vtable
{
  int (*add_ref)   __P ((stream_t));
  int (*release)   __P ((stream_t));
  int (*destroy)   __P ((stream_t));

  int (*open)      __P ((stream_t, const char *, int, int));
  int (*close)     __P ((stream_t));

  int (*read)      __P ((stream_t, void *, size_t, size_t *));
  int (*readline)  __P ((stream_t, char *, size_t, size_t *));
  int (*write)     __P ((stream_t, const void *, size_t, size_t *));

  int (*seek)      __P ((stream_t, off_t, enum stream_whence));
  int (*tell)      __P ((stream_t, off_t *));

  int (*get_size)  __P ((stream_t, off_t *));
  int (*truncate)  __P ((stream_t, off_t));
  int (*flush)     __P ((stream_t));

  int (*get_fd)    __P ((stream_t , int *));
  int (*get_flags) __P ((stream_t, int *));
  int (*get_state) __P ((stream_t, enum stream_state *));
};

struct _stream
{
  struct _stream_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_STREAM_H */
