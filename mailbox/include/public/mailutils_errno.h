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

/*
 * The errno <errno.h> is not accurate enough.
 * We try the approach of defining our own set
 * of errors.  Since all of the mailbox related
 * functions have the error as a return value
 * this make things easier and not dependent
 * of any global variable.
 * We use the MU_ERROR_* > 512  for  our range.
 */

#ifndef _MAILUTILS_ERRNO_H
#define _MAILUTILS_ERRNO_H

#include <errno.h>

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


/* No error */
#define MU_ERROR_OK               0

/* Permission denied */
#define MU_ERROR_ACCESS           EACCES

/* Resource temporarily unavailable */
#define MU_ERROR_TRY_AGAIN        EAGAIN

/* Resource temporarily unavailable */
#define MU_ERROR_IN_PROGRESS      EINPROGRESS

/* Resource busy */
#define MU_ERROR_BUSY             EBUSY

/* Operation canceled */
#define MU_ERROR_CANCELED         ECANCELED

/* Resource deadlock avoided */
#define MU_ERROR_DEAD_LOCK        EDEADLK

/* Interrupted function call */
#define MU_ERROR_INTR_CALL        EINTR

/* Invalid argument */
#define MU_ERROR_INVALID_ARG      EINVAL

/* Input/output error */
#define MU_ERROR_IO               EIO

/* Inappropriate message buffer length */
#define MU_ERROR_INVALID_SIZE     EMSGSIZE

/* No locks available */
#define MU_ERROR_NO_LOCK          ENOLCK

/* Function not implemented */
#define MU_ERROR_NOT_IMPLEMENTED  ENOSYS

/* Not supported */
#define MU_ERROR_NOT_SUPPORTED    ENOTSUP

/* Operation timed out */
#define MU_ERROR_TIMEDOUT         ETIMEOUT

/* define bad handle */
#define MU_ERROR_BAD_HANDLE       EBADF

/* no such entry */
#define MU_ERROR_NO_ENTRY         ENOENT

/* unknown host */
#define MU_ERROR_OUT_OF_MEMORY    ENOMEM

/* unknown host */
#define MU_ERROR_UKNOWN_HOST      529

extern const char * mailutils_error __P ((int err));
extern void mailutils_perror        __P ((int err, const char *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX_H */
