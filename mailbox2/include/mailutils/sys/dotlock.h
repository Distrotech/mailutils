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

#ifndef _MAILUTILS_SYS_DOTLOCK_H
#define _MAILUTILS_SYS_DOTLOCK_H

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#include <mailutils/refcount.h>
#include <mailutils/sys/lockfile.h>

#ifdef __cplusplus
extern "C" {
#endif

/* locking flags */
#define MU_LOCKFILE_DOTLOCK_PID    1
#define MU_LOCKFILE_DOTLOCK_FCNTL  2
#define MU_LOCKFILE_DOTLOCK_TIME   4

#define MU_LOCKFILE_DOTLOCK_EXPIRE_TIME        (5 * 60)

#define MU_LOCKFILE_DOTLOCK_ATTR           0444

struct _lockfile_dotlock
{
  struct _lockfile base;
  mu_refcount_t refcount;
  int fd;
  int refcnt; /* For recursive file locking.  */
  char *fname;
  int flags;
};

extern int  _lockfile_dotlock_ctor __P ((struct _lockfile_dotlock *, const char *));
extern void _lockfile_dotlock_dtor __P ((struct _lockfile_dotlock *));
extern int  _lockfile_dotlock_ref  __P ((lockfile_t));
extern void _lockfile_dotlock_destroy __P ((lockfile_t *));

extern int  _lockfile_dotlock_lock    __P ((lockfile_t));
extern int  _lockfile_dotlock_touchlock __P ((lockfile_t));
extern int  _lockfile_dotlock_unlock    __P ((lockfile_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_DOTLOCK_H */
