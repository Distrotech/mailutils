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

#ifndef _MAILUTILS_LOCKER_H
#define _MAILUTILS_LOCKER_H

#include <mailutils/mu_features.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _locker;
typedef struct _locker *locker_t;

extern int locker_create   __P ((locker_t *, const char *filename,
				 size_t len, int flags));
extern void locker_destroy __P ((locker_t *));

#define MU_LOCKER_RDLOCK 0
#define MU_LOCKER_WRLOCK 1

/* locking flags */
#define MU_LOCKER_PID    1
#define MU_LOCKER_FCNTL  2
#define MU_LOCKER_TIME   4

#define MU_LOCKER_EXPIRE_TIME        (5 * 60)

extern int locker_lock          __P ((locker_t, int flag));
extern int locker_touchlock     __P ((locker_t));
extern int locker_unlock        __P ((locker_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MAILBOX_H */
