/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_DBM_H
# define _MAILUTILS_SYS_DBM_H

union _mu_dbm_errno
{
  int n;
  void *p;
};

struct _mu_dbm_file
{
  char *db_name;              /* Database name */
  void *db_descr;             /* Database descriptor */
  int db_safety_flags;        /* Safety checks */
  uid_t db_owner;             /* Database owner UID */
  struct mu_dbm_impl *db_sys; /* Pointer to the database implementation */
  union _mu_dbm_errno db_errno;
};

#endif
