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

#ifndef _MAILUTILS_ERROR_H
#define _MAILUTILS_ERROR_H

#include <errno.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef __cplusplus
extern "C" {
#endif

/* When Possible use the platform error for our needs.  */
#ifndef MU_ERROR_RANGE
# define MU_ERROR_RANGE 600
#endif

/* Error codes.  */
#define MU_ERROR_OK                           0

#if defined (EINVAL)
# define MU_ERROR_INVALID_PARAMETER EINVAL
#else
# define MU_ERROR_INVALID_PARAMETER (MU_ERROR_RANGE + 1)
#endif

#if defined (ENOMEM)
# define MU_ERROR_NO_MEMORY ENOMEM
#else
# define MU_ERROR_NO_MEMORY (MU_ERROR_RANGE + 2)
#endif

#if defined (EIO)
# define MU_ERROR_IO EIO
#else
# define MU_ERROR_IO (MU_ERROR_RANGE + 3)
#endif

#if defined (EINTR)
# define MU_ERROR_INTERRUPT EINTR
#else
# define MU_ERROR_INTERRUPT (MU_ERROR_RANGE + 4)
#endif

#if defined (ENOSYS)
# define MU_ERROR_NOT_SUPPORTED ENOSYS
#else
# define MU_ERROR_NOT_SUPPORTED (MU_ERROR_RANGE + 5)
#endif

#if defined (EACCESS)
# define MU_ERROR_OPERATION_DENIED EACCESS
#else
# define MU_ERROR_OPERATION_DENIED (MU_ERROR_RANGE + 6)
#endif

#if defined (ETIMEOUT)
# define MU_ERROR_TIMEOUT ETIMEOUT
#else
# define MU_ERROR_TIMEOUT (MU_ERROR_RANGE + 7)
#endif

#if defined (EAGAIN)
# define MU_ERROR_TRY_AGAIN EAGAIN
#else
# define MU_ERROR_TRY_AGAIN (MU_ERROR_RANGE + 8)
#endif

#if defined (ENOLCK)
# define MU_ERROR_NO_LOCK ENOLCK
#else
# define MU_ERROR_NO_LOCK (MU_ERROR_RANGE + 9)
#endif

#if defined (EINPROGRESS)
# define MU_ERROR_OPERATION_IN_PROGRESS EINPROGRESS
#else
# define MU_ERROR_OPERATION_IN_PROGRESS (MU_ERROR_RANGE + 10)
#endif

#if defined (EBUSY)
# define MU_ERROR_RESOURCE_BUSY EBUSY
#else
# define MU_ERROR_RESOURCE_BUSY (MU_ERROR_RANGE + 11)
#endif

#if defined (ESPIPE)
# define MU_ERROR_INVALID_SEEK ESPIPE
#else
# define MU_ERROR_INVALID_SEEK (MU_ERROR_RANGE + 12)
#endif

#if defined (ECANCELED)
# define MU_ERROR_OPERATION_CANCELED ECANCELED
#else
# define MU_ERROR_OPERATION_CANCELED (MU_ERROR_RANGE + 13)
#endif

#if defined (EBADF)
# define MU_ERROR_FD_INVALID EBADF
#else
# define MU_ERROR_FD_INVALID (MU_ERROR_RANGE + 14)
#endif

#if defined (ENOENT)
# define MU_ERROR_ENTRY_NOT_EXIST ENOENT
#else
# define MU_ERROR_ENTRY_NOT_EXIST (MU_ERROR_RANGE + 15)
#endif

#if defined (EEXIST)
# define MU_ERROR_ENTRY_EXIST EEXIST
#else
# define MU_ERROR_ENTRY_EXIST (MU_ERROR_RANGE + 16)
#endif

#if defined (ENOTDIR)
# define MU_ERROR_NO_DIRECTORY ENOTDIR
#else
# define MU_ERROR_NO_DIRECTORY (MU_ERROR_RANGE + 17)
#endif


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ERROR_H */
