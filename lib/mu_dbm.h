/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <mailutils/stream.h>

#if defined(WITH_GDBM)

#include <gdbm.h>
#define USE_DBM
typedef GDBM_FILE DBM_FILE;
typedef datum DBM_DATUM;
#define MU_DATUM_SIZE(d) d.dsize
#define MU_DATUM_PTR(d) d.dptr

#elif defined(WITH_BDB2)

#include <db.h>
#define USE_DBM

struct db2_file
{
  DB *db;
  DBC *dbc;
};

typedef struct db2_file *DBM_FILE;
typedef DBT DBM_DATUM;
#define MU_DATUM_SIZE(d) d.size
#define MU_DATUM_PTR(d) d.data

#elif defined(WITH_NDBM)

#include <ndbm.h>
#define USE_DBM
typedef DBM *DBM_FILE;
typedef datum DBM_DATUM;
#define MU_DATUM_SIZE(d) d.dsize
#define MU_DATUM_PTR(d) d.dptr

#elif defined(WITH_OLD_DBM)

#include <dbm.h>
#define USE_DBM
typedef int DBM_FILE;
typedef datum DBM_DATUM;
#define MU_DATUM_SIZE(d) d.dsize
#define MU_DATUM_PTR(d) d.dptr

#endif

#ifdef USE_DBM
struct stat;
int mu_dbm_stat (char *name, struct stat *sb);
int mu_dbm_open __P((char *name, DBM_FILE *db, int flags, int mode));
int mu_dbm_close __P((DBM_FILE db));
int mu_dbm_fetch __P((DBM_FILE db, DBM_DATUM key, DBM_DATUM *ret));
int mu_dbm_insert __P((DBM_FILE db, DBM_DATUM key, DBM_DATUM contents, int replace));
int mu_dbm_delete (DBM_FILE db, DBM_DATUM key);
DBM_DATUM mu_dbm_firstkey __P((DBM_FILE db));
DBM_DATUM mu_dbm_nextkey __P((DBM_FILE db, DBM_DATUM key));
#endif

int mu_fcheck_perm __P((int fd, int mode));
int mu_check_perm __P((const char *name, int mode));
