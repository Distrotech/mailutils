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

#ifndef _MAILUTILS_LOCKFILE_H
#define _MAILUTILS_LOCKFILE_H

#include <sys/types.h>
#include <mailutils/mu_features.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _lockfile;
typedef struct _lockfile *lockfile_t;

extern int  lockfile_ref            __P ((lockfile_t));
extern void lockfile_destroy        __P ((lockfile_t *));

extern int  lockfile_lock           __P ((lockfile_t));
extern int  lockfile_touchlock      __P ((lockfile_t));
extern int  lockfile_unlock         __P ((lockfile_t));

extern int  lockfile_dotlock_create __P ((lockfile_t *, const char *filename));
extern int  lockfile_nfslock_create __P ((lockfile_t *, const char *filename));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_LOCKFILE_H */
