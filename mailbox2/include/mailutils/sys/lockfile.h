/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_SYS_LOCKFILE_H
#define _MAILUTILS_SYS_LOCKFILE_H

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <mailutils/lockfile.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _lockfile_vtable
{
  int  (*ref)       __P ((lockfile_t));
  void (*destroy)   __P ((lockfile_t *));

  int  (*lock)      __P ((lockfile_t));
  int  (*touchlock) __P ((lockfile_t));
  int  (*unlock)    __P ((lockfile_t));
};

struct _lockfile
{
  struct _lockfile_vtable *vtable;
};

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_LOCKFILE_H */
