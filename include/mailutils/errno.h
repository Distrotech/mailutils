/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_ERRNO_H
#define _MAILUTILS_ERRNO_H	1

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# if __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*!__P */

#define MU_ERR_BASE            0x1000

#define MU_ERR_NO_HANDLER      (MU_ERR_BASE | 0x001)
#define MU_ERR_EMPTY_VFN       (MU_ERR_BASE | 0x002)

#define MU_ERR_OUT_NULL        (MU_ERR_BASE | 0x003)
#define MU_ERR_OUT_PTR_NULL    (MU_ERR_BASE | 0x004)

#define MU_ERR_MBX_NULL        (MU_ERR_BASE | 0x005)

#define MU_ERR_BAD_822_FORMAT  (MU_ERR_BASE | 0x006)
#define MU_ERR_EMPTY_ADDRESS   (MU_ERR_BASE | 0x007)

#define MU_ERR_LOCKER_NULL     (MU_ERR_BASE | 0x008)
#define MU_ERR_LOCK_CONFLICT   (MU_ERR_BASE | 0x009)
#define MU_ERR_LOCK_BAD_LOCK   (MU_ERR_BASE | 0x00a)
#define MU_ERR_LOCK_BAD_FILE   (MU_ERR_BASE | 0x00b)
#define MU_ERR_LOCK_NOT_HELD   (MU_ERR_BASE | 0x00c)

const char* mu_errname __P((int e));
const char* mu_errstring __P((int e));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ERRNO_H */

