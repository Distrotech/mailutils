/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_LOCKFILE_H
#define _MAILUTILS_LOCKFILE_H

#include <sys/types.h>
#include <mailutils/mu_features.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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
