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

#include <string.h>

#include <mailutils/errno.h>

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
    EN(MU_ERR_NO_PTR)

    EN(MU_ERR_MBX_NULL)
    EN(MU_ERR_MBX_PTR_NULL)
  }

  return "SYSTEM ERROR";
}

const char* mu_errstr (int e)
{
  switch(e)
  {
#define ES(x, d)  case x: return d;
    ES(MU_ERR_NO_HANDLER,     "No registered handler")
    ES(MU_ERR_EMPTY_VFN,      "Empty virtual function")
    ES(MU_ERR_NO_PTR,         "No pointer")

    ES(MU_ERR_MBX_NULL,       "Mailbox null")
    ES(MU_ERR_MBX_PTR_NULL,   "Mailbox pointer null")
  }

  return strerror(e);
}


