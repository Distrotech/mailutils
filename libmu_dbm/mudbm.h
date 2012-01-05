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

#include <mailutils/sys/dbm.h>

#define DBMSYSCK(db,meth) do					\
    {								\
      if (!(db))						\
	return EINVAL;						\
      if (!(db)->db_descr)					\
	return MU_ERR_NOT_OPEN;					\
      if (!(db)->db_sys || !(db)->db_sys->meth)			\
	return ENOSYS;						\
    }								\
  while (0)

#ifdef WITH_GDBM
extern struct mu_dbm_impl _mu_dbm_gdbm;
#endif
#ifdef WITH_BDB
extern struct mu_dbm_impl _mu_dbm_bdb;
#endif
#ifdef WITH_NDBM
extern struct mu_dbm_impl _mu_dbm_ndbm;
#endif
#ifdef WITH_TOKYOCABINET
extern struct mu_dbm_impl _mu_dbm_tokyocabinet;
#endif
#ifdef WITH_KYOTOCABINET
extern struct mu_dbm_impl _mu_dbm_kyotocabinet;
#endif

void _mu_dbm_init (void);


