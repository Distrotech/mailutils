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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/dbm.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>
#include <mailutils/io.h>
#include "mudbm.h"

#if defined(WITH_NDBM)
#include <ndbm.h>

static int
_ndbm_file_safety (mu_dbm_file_t db, int mode, uid_t owner)
{
  int rc;
  char *name;
  
  rc = mu_asprintf (&name, "%s.pag", db->db_name);
  if (rc)
    return rc;
  rc = mu_file_safety_check (name, mode, owner, NULL);
  if (rc)
    {
      free (name);
      return rc;
    }

  strcpy (name + strlen (name) - 3, "dir");
  rc = mu_file_safety_check (name, mode, owner, NULL);
  free (name);
  return rc;
}

int
_ndbm_get_fd (mu_dbm_file_t db, int *pag, int *dir)
{
  DBM *dbm = db->db_descr;
  *pag = dbm_pagfno (dbm);
  if (dir)
    *dir = dbm_dirfno (dbm);
  return 0;
}

static int
_ndbm_open (mu_dbm_file_t db, int flags, int mode)
{
  int f;
  DBM *dbm;
  
  switch (flags)
    {
    case MU_STREAM_CREAT:
      f = O_CREAT|O_TRUNC|O_RDWR;
      break;
      
    case MU_STREAM_READ:
      f = O_RDONLY;
      break;
      
    case MU_STREAM_RDWR:
      f = O_CREAT|O_RDWR;
      break;
      
    default:
      errno = EINVAL;
      return -1;
    }
  dbm = dbm_open (db->db_name, f, mode);
  if (!dbm)
    return MU_ERR_FAILURE;
  db->db_descr = dbm;
  return 0;
}

static int
_ndbm_close (mu_dbm_file_t db)
{
  if (db->db_descr)
    {
      dbm_close ((DBM *) db->db_descr);
      db->db_descr = NULL;
    }
  return 0;
}

static int
_ndbm_conv_datum (mu_dbm_file_t db, struct mu_dbm_datum *ret, datum content)
{
  ret->mu_dptr = malloc (content.dsize);
  if (!ret->mu_dptr)
    return errno;
  memcpy (ret->mu_dptr, content.dptr, content.dsize);
  ret->mu_dsize = content.dsize;
  ret->mu_sys = db->db_sys;
  return 0;
}

static int
_ndbm_fetch (mu_dbm_file_t db, struct mu_dbm_datum const *key,
	     struct mu_dbm_datum *ret)
{
  datum keydat, content;

  keydat.dptr = key->mu_dptr;
  keydat.dsize = key->mu_dsize;
  errno = 0;
  content = dbm_fetch (db->db_descr, keydat);
  mu_dbm_datum_free (ret);
  if (content.dptr == NULL)
    return MU_ERR_NOENT;
  return _ndbm_conv_datum (db, ret, content);
}

static int
_ndbm_store (mu_dbm_file_t db,
	     struct mu_dbm_datum const *key,
	     struct mu_dbm_datum const *contents,
	     int replace)
{
  DBM *dbm = db->db_descr;
  datum keydat, condat;
  
  keydat.dptr = key->mu_dptr;
  keydat.dsize = key->mu_dsize;
  condat.dptr = contents->mu_dptr;
  condat.dsize = contents->mu_dsize;
  errno = 0;
  switch (dbm_store (dbm, keydat, condat,
		     replace ? DBM_REPLACE : DBM_INSERT))
    {
    case 0:
      break;
      
    case 1:
      return MU_ERR_EXISTS;
      
    case -1:
      db->db_errno.n = errno;
      return MU_ERR_FAILURE;
    }
  return 0;
}

static int
_ndbm_delete (mu_dbm_file_t db, struct mu_dbm_datum const *key)
{
  DBM *dbm = db->db_descr;
  datum keydat;

  keydat.dptr = key->mu_dptr;
  keydat.dsize = key->mu_dsize;
  errno = 0;
  switch (dbm_delete (dbm, keydat))
    {
    case 0:
      break;
      
    case 1:
      return MU_ERR_NOENT;
      
    case -1:
      db->db_errno.n = errno;
      return MU_ERR_FAILURE;
    }
  return 0;
}

static int
_ndbm_firstkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  DBM *dbm = db->db_descr;
  datum keydat;

  errno = 0;
  keydat = dbm_firstkey (dbm);
  if (keydat.dptr == NULL)
    return MU_ERR_NOENT;
  return _ndbm_conv_datum (db, ret, keydat);
}

static int
_ndbm_nextkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  DBM *dbm = db->db_descr;
  datum keydat;

  keydat = dbm_nextkey (dbm);
  if (keydat.dptr == NULL)
    return MU_ERR_NOENT;
  return _ndbm_conv_datum (db, ret, keydat);
}

static void
_ndbm_datum_free (struct mu_dbm_datum *datum)
{
  free (datum->mu_dptr);
}

static char const *
_ndbm_strerror (mu_dbm_file_t db)
{
  return strerror (db->db_errno.n);
}

struct mu_dbm_impl _mu_dbm_ndbm = {
  "ndbm",
  _ndbm_file_safety,
  _ndbm_get_fd,
  _ndbm_open,
  _ndbm_close,
  _ndbm_fetch,
  _ndbm_store,
  _ndbm_delete,
  _ndbm_firstkey,
  _ndbm_nextkey,
  _ndbm_datum_free,
  _ndbm_strerror
};
  
#endif
