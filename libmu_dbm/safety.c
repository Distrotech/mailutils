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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/types.h>
#include <mailutils/dbm.h>
#include <mailutils/errno.h>
#include "mudbm.h"

int
mu_dbm_safety_set_owner (mu_dbm_file_t db, uid_t uid)
{
  if (!db)
    return EINVAL;
  db->db_owner = uid;
  return 0;
}

int
mu_dbm_safety_get_owner (mu_dbm_file_t db, uid_t *uid)
{
  if (!db)
    return EINVAL;
  *uid = db->db_owner;
  return 0;
}

int
mu_dbm_safety_set_flags (mu_dbm_file_t db, int flags)
{
  if (!db)
    return EINVAL;
  db->db_safety_flags = flags;
  return 0;
}

int
mu_dbm_safety_get_flags (mu_dbm_file_t db, int *flags)
{
  if (!db)
    return EINVAL;
  *flags = db->db_safety_flags;
  return 0;
}
  
int
mu_dbm_safety_check (mu_dbm_file_t db)
{
  if (!db)
    return EINVAL;
  if (!db->db_sys || !db->db_sys->_dbm_file_safety)
    return ENOSYS;
  return db->db_sys->_dbm_file_safety (db, db->db_safety_flags,
				       db->db_owner);
}
