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

#ifndef _MAILUTILS_ERRNO_H
#define _MAILUTILS_ERRNO_H	1

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_ERR_BASE            0x1000

#define MU_ERR_NO_HANDLER           (MU_ERR_BASE | 0x001)
#define MU_ERR_EMPTY_VFN            (MU_ERR_BASE | 0x002)

#define MU_ERR_OUT_NULL             (MU_ERR_BASE | 0x003)
#define MU_ERR_OUT_PTR_NULL         (MU_ERR_BASE | 0x004)

#define MU_ERR_MBX_NULL             (MU_ERR_BASE | 0x005)

#define MU_ERR_BAD_822_FORMAT       (MU_ERR_BASE | 0x006)
#define MU_ERR_EMPTY_ADDRESS        (MU_ERR_BASE | 0x007)

#define MU_ERR_LOCKER_NULL          (MU_ERR_BASE | 0x008)
#define MU_ERR_LOCK_CONFLICT        (MU_ERR_BASE | 0x009)
#define MU_ERR_LOCK_BAD_LOCK        (MU_ERR_BASE | 0x00a)
#define MU_ERR_LOCK_BAD_FILE        (MU_ERR_BASE | 0x00b)
#define MU_ERR_LOCK_NOT_HELD        (MU_ERR_BASE | 0x00c)
#define MU_ERR_LOCK_EXT_FAIL        (MU_ERR_BASE | 0x00d)
  /* external program not found, or lack of system resources */
#define MU_ERR_LOCK_EXT_ERR         (MU_ERR_BASE | 0x00e)
  /* external program failed, specific reason unknown */
#define MU_ERR_LOCK_EXT_KILLED      (MU_ERR_BASE | 0x00f)
  /* external program was interrupted */

#define MU_ERR_NO_SUCH_USER         (MU_ERR_BASE | 0x010)

#define MU_ERR_GETHOSTBYNAME        (MU_ERR_BASE | 0x011)

#define MU_ERR_BAD_RESUMPTION       (MU_ERR_BASE | 0x012)

#define MU_ERR_MAILER_BAD_FROM      (MU_ERR_BASE | 0x013)
#define MU_ERR_MAILER_BAD_TO        (MU_ERR_BASE | 0x014)
#define MU_ERR_MAILER_NO_RCPT_TO    (MU_ERR_BASE | 0x015)
#define MU_ERR_MAILER_BAD_URL       (MU_ERR_BASE | 0x016)
#define MU_ERR_SMTP_RCPT_FAILED     (MU_ERR_BASE | 0x017)

#define MU_ERR_TCP_NO_HOST          (MU_ERR_BASE | 0x018)
#define MU_ERR_TCP_NO_PORT          (MU_ERR_BASE | 0x019)

#define MU_ERR_BAD_2047_INPUT       (MU_ERR_BASE | 0x020)
  
const char* mu_errname __P((int e));
const char* mu_strerror __P((int e));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ERRNO_H */

