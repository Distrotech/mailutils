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

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* lock expiry time */
#define MU_LOCKER_EXPIRE_TIME        (10 * 60)
#define MU_LOCKER_RETRIES            (10)
#define MU_LOCKER_RETRY_SLEEP        (1)
#define MU_LOCKER_EXTERNAL_PROGRAM   "dotlock"

/* return codes for the external locker */
#define MU_DL_EX_PERM    4 /* insufficient permissions */
#define MU_DL_EX_EXIST   3 /* lock requested, but file is already locked */
#define MU_DL_EX_NEXIST  2 /* unlock requested, but file is not locked */
#define MU_DL_EX_ERROR   1 /* failed due to some other error */
#define MU_DL_EX_OK      0 /* success */

/* locker_create() flags */

#define MU_LOCKER_SIMPLE   0x00
  /* Just try and dotlock the file, not the default because its usually
     better to retry. */
#define MU_LOCKER_RETRY    0x01
  /* This requests that we loop retries times, sleeping retry_sleep
     seconds in between trying to obtain the lock before failing with
     MU_LOCK_CONFLICT. */
#define MU_LOCKER_TIME     0x02
  /* This mode checks the last update time of the lock, then removes
     it if older than MU_LOCKER_EXPIRE_TIME. If a client uses this,
     then the servers better periodically update the lock on the
     file... do they? */
#define MU_LOCKER_PID      0x04
  /* PID locking is only useful for programs that aren't using
     an external dotlocker, non-setgid programs will use a dotlocker,
     which locks and exits imediately. This is a protection against
     a server crashing, it's not generally useful. */
#define MU_LOCKER_EXTERNAL 0x08
  /* Use an external program to lock the file. This is necessary
     for programs having permission to access a file, but do not
     have write permission on the directory that contains that file. */
#define MU_LOCKER_NULL     0x10
  /* Special locker type: means no lock. This is to be used with
     temporary mailboxes stored in memory. */

#define MU_LOCKER_DEFAULT  (MU_LOCKER_RETRY)

extern int locker_create __P ((locker_t *, const char *filename, int flags));
extern void locker_destroy __P ((locker_t *));

/* Time is measured in seconds. */

extern int locker_set_flags __P ((locker_t, int));
extern int locker_set_expire_time __P ((locker_t, int));
extern int locker_set_retries __P ((locker_t, int));
extern int locker_set_retry_sleep __P ((locker_t, int));
extern int locker_set_external __P ((locker_t, const char* program));

extern int locker_get_flags __P ((locker_t, int*));
extern int locker_get_expire_time __P ((locker_t, int*));
extern int locker_get_retries __P ((locker_t, int*));
extern int locker_get_retry_sleep __P ((locker_t, int*));
extern int locker_get_external __P ((locker_t, char**));

extern int locker_lock          __P ((locker_t));
extern int locker_touchlock     __P ((locker_t));
extern int locker_unlock        __P ((locker_t));
extern int locker_remove_lock   __P ((locker_t));

#ifdef __cplusplus
}
#endif

#endif

