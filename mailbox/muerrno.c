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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <string.h>

#include <mailutils/errno.h>

#ifndef EOK
# define EOK 0
#endif

const char*
mu_errname (int e)
{
  switch(e)
  {
#define EN(x)  case x: return #x;

    EN(EOK)
    EN(EPERM)
    EN(ENOENT)
    EN(ESRCH)
    EN(EINTR)
    EN(EAGAIN)
    EN(ENOMEM)
    EN(EEXIST)
    EN(EINVAL)

    EN(MU_ERR_NO_HANDLER)
    EN(MU_ERR_EMPTY_VFN)

    EN(MU_ERR_OUT_NULL)
    EN(MU_ERR_OUT_PTR_NULL)

    EN(MU_ERR_MBX_NULL)

    EN(MU_ERR_BAD_822_FORMAT)
    EN(MU_ERR_EMPTY_ADDRESS)

    EN(MU_ERR_LOCKER_NULL)
    EN(MU_ERR_LOCK_CONFLICT)
    EN(MU_ERR_LOCK_BAD_LOCK)
    EN(MU_ERR_LOCK_BAD_FILE)
    EN(MU_ERR_LOCK_NOT_HELD)
    EN(MU_ERR_LOCK_EXT_FAIL)
    EN(MU_ERR_LOCK_EXT_ERR)
    EN(MU_ERR_LOCK_EXT_KILLED)

    EN(MU_ERR_NO_SUCH_USER)
    EN(MU_ERR_GETHOSTBYNAME)
    EN(MU_ERR_BAD_RESUMPTION)
    EN(MU_ERR_MAILER_BAD_FROM)
    EN(MU_ERR_MAILER_BAD_TO)
    EN(MU_ERR_MAILER_BAD_URL)
    EN(MU_ERR_MAILER_NO_RCPT_TO)
    EN(MU_ERR_SMTP_RCPT_FAILED)
    EN(MU_ERR_TCP_NO_HOST)
    EN(MU_ERR_TCP_NO_PORT)
  }

  return "SYSTEM ERROR";
}

const char* mu_errstring (int e)
{
  switch(e)
  {
#define ES(x, d)  case x: return d;
    ES(EOK,                     "Success")

    ES(MU_ERR_NO_HANDLER,       "No registered handler")
    ES(MU_ERR_EMPTY_VFN,        "Empty virtual function")

    ES(MU_ERR_OUT_NULL,         "Pointer to output null")
    ES(MU_ERR_OUT_PTR_NULL,     "Pointer to output pointer null")

    ES(MU_ERR_MBX_NULL,         "Mailbox null")

    ES(MU_ERR_BAD_822_FORMAT,   "Format of RFC822 object is bad")
    ES(MU_ERR_EMPTY_ADDRESS,    "Address contains no addr specs")

    ES(MU_ERR_LOCKER_NULL,      "Locker null")
    ES(MU_ERR_LOCK_CONFLICT,    "Conflict with previous locker")
    ES(MU_ERR_LOCK_BAD_LOCK,    "Lock file check failed")
    ES(MU_ERR_LOCK_BAD_FILE,    "File check failed")
    ES(MU_ERR_LOCK_NOT_HELD,    "Lock not held on file")
    ES(MU_ERR_LOCK_EXT_FAIL,    "Failed to exec external locker")
    ES(MU_ERR_LOCK_EXT_ERR,     "External locker failed")
    ES(MU_ERR_LOCK_EXT_KILLED,  "External locker killed")

    ES(MU_ERR_NO_SUCH_USER,     "No such user name")

    ES(MU_ERR_GETHOSTBYNAME,    "DNS name resolution failed")
    ES(MU_ERR_BAD_RESUMPTION,   "State busy, must resume operation")
    ES(MU_ERR_MAILER_BAD_FROM,  "Not a valid mailer from address")
    ES(MU_ERR_MAILER_BAD_TO,    "Not a valid mailer to address")
    ES(MU_ERR_MAILER_NO_RCPT_TO,"No receipt addresses found")
    ES(MU_ERR_MAILER_BAD_URL,   "Malformed or unsupported mailer URL")
    ES(MU_ERR_SMTP_RCPT_FAILED, "SMTP rcpt to command failed")
    ES(MU_ERR_TCP_NO_HOST,      "Tcp connections need a host")
    ES(MU_ERR_TCP_NO_PORT,      "Tcp connections need a postive port")
  }

  return strerror(e);
}


