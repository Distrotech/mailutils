/* -*- c -*- This file is generated automatically. Please, do not edit.
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003-2005, 2007, 2010-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_ERRNO_H
#define _MAILUTILS_ERRNO_H	1

#include <errno.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_ERR_BASE                 0x1000

#define MU_ERR_FAILURE (MU_ERR_BASE+0)
#define MU_ERR_CANCELED (MU_ERR_BASE+1)
#define MU_ERR_EMPTY_VFN (MU_ERR_BASE+2)
#define MU_ERR_OUT_PTR_NULL (MU_ERR_BASE+3)
#define MU_ERR_MBX_REMOVED (MU_ERR_BASE+4)
#define MU_ERR_NOT_OPEN (MU_ERR_BASE+5)
#define MU_ERR_OPEN (MU_ERR_BASE+6)
#define MU_ERR_INVALID_EMAIL (MU_ERR_BASE+7)
#define MU_ERR_EMPTY_ADDRESS (MU_ERR_BASE+8)
#define MU_ERR_LOCKER_NULL (MU_ERR_BASE+9)
#define MU_ERR_LOCK_CONFLICT (MU_ERR_BASE+10)
#define MU_ERR_LOCK_BAD_LOCK (MU_ERR_BASE+11)
#define MU_ERR_LOCK_BAD_FILE (MU_ERR_BASE+12)
#define MU_ERR_LOCK_NOT_HELD (MU_ERR_BASE+13)
#define MU_ERR_LOCK_EXT_FAIL (MU_ERR_BASE+14)
#define MU_ERR_LOCK_EXT_ERR (MU_ERR_BASE+15)
#define MU_ERR_LOCK_EXT_KILLED (MU_ERR_BASE+16)
#define MU_ERR_NO_SUCH_USER (MU_ERR_BASE+17)
#define MU_ERR_GETHOSTBYNAME (MU_ERR_BASE+18)
#define MU_ERR_MAILER_BAD_FROM (MU_ERR_BASE+19)
#define MU_ERR_MAILER_BAD_TO (MU_ERR_BASE+20)
#define MU_ERR_MAILER_NO_RCPT_TO (MU_ERR_BASE+21)
#define MU_ERR_MAILER_BAD_URL (MU_ERR_BASE+22)
#define MU_ERR_SMTP_RCPT_FAILED (MU_ERR_BASE+23)
#define MU_ERR_TCP_NO_HOST (MU_ERR_BASE+24)
#define MU_ERR_TCP_NO_PORT (MU_ERR_BASE+25)
#define MU_ERR_BAD_2047_INPUT (MU_ERR_BASE+26)
#define MU_ERR_BAD_2047_ENCODING (MU_ERR_BASE+27)
#define MU_ERR_NOUSERNAME (MU_ERR_BASE+28)
#define MU_ERR_NOPASSWORD (MU_ERR_BASE+29)
#define MU_ERR_BADREPLY (MU_ERR_BASE+30)
#define MU_ERR_SEQ (MU_ERR_BASE+31)
#define MU_ERR_REPLY (MU_ERR_BASE+32)
#define MU_ERR_BAD_AUTH_SCHEME (MU_ERR_BASE+33)
#define MU_ERR_AUTH_FAILURE (MU_ERR_BASE+34)
#define MU_ERR_PROCESS_NOEXEC (MU_ERR_BASE+35)
#define MU_ERR_PROCESS_EXITED (MU_ERR_BASE+36)
#define MU_ERR_PROCESS_SIGNALED (MU_ERR_BASE+37)
#define MU_ERR_PROCESS_UNKNOWN_FAILURE (MU_ERR_BASE+38)
#define MU_ERR_CONN_CLOSED (MU_ERR_BASE+39)
#define MU_ERR_PARSE (MU_ERR_BASE+40)
#define MU_ERR_NOENT (MU_ERR_BASE+41)
#define MU_ERR_EXISTS (MU_ERR_BASE+42)
#define MU_ERR_BUFSPACE (MU_ERR_BASE+43)
#define MU_ERR_SQL (MU_ERR_BASE+44)
#define MU_ERR_DB_ALREADY_CONNECTED (MU_ERR_BASE+45)
#define MU_ERR_DB_NOT_CONNECTED (MU_ERR_BASE+46)
#define MU_ERR_RESULT_NOT_RELEASED (MU_ERR_BASE+47)
#define MU_ERR_NO_QUERY (MU_ERR_BASE+48)
#define MU_ERR_BAD_COLUMN (MU_ERR_BASE+49)
#define MU_ERR_NO_RESULT (MU_ERR_BASE+50)
#define MU_ERR_NO_INTERFACE (MU_ERR_BASE+51)
#define MU_ERR_BADOP (MU_ERR_BASE+52)
#define MU_ERR_BAD_FILENAME (MU_ERR_BASE+53)
#define MU_ERR_READ (MU_ERR_BASE+54)
#define MU_ERR_NO_TRANSPORT (MU_ERR_BASE+55)
#define MU_ERR_AUTH_NO_CRED (MU_ERR_BASE+56)
#define MU_ERR_URL_MISS_PARTS (MU_ERR_BASE+57)
#define MU_ERR_URL_EXTRA_PARTS (MU_ERR_BASE+58)
#define MU_ERR_URL_INVALID_PARAMETER (MU_ERR_BASE+59)
#define MU_ERR_INFO_UNAVAILABLE (MU_ERR_BASE+60)
#define MU_ERR_NONAME (MU_ERR_BASE+61)
#define MU_ERR_BADFLAGS (MU_ERR_BASE+62)
#define MU_ERR_SOCKTYPE (MU_ERR_BASE+63)
#define MU_ERR_FAMILY (MU_ERR_BASE+64)
#define MU_ERR_SERVICE (MU_ERR_BASE+65)
#define MU_ERR_PERM_OWNER_MISMATCH (MU_ERR_BASE+66)
#define MU_ERR_PERM_GROUP_WRITABLE (MU_ERR_BASE+67)
#define MU_ERR_PERM_WORLD_WRITABLE (MU_ERR_BASE+68)
#define MU_ERR_PERM_GROUP_READABLE (MU_ERR_BASE+69)
#define MU_ERR_PERM_WORLD_READABLE (MU_ERR_BASE+70)
#define MU_ERR_PERM_LINKED_WRDIR (MU_ERR_BASE+71)
#define MU_ERR_PERM_DIR_IWGRP (MU_ERR_BASE+72)
#define MU_ERR_PERM_DIR_IWOTH (MU_ERR_BASE+73)
#define MU_ERR_DISABLED (MU_ERR_BASE+74)
#define MU_ERR_FORMAT (MU_ERR_BASE+75)
#define MU_ERR_LAST (MU_ERR_BASE+76)

#ifndef ECANCELED
# define ECANCELED MU_ERR_CANCELED
#endif  
  
const char *mu_errname (int e);
const char *mu_strerror (int e);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ERRNO_H */

