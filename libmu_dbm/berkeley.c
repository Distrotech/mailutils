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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mailutils/types.h>
#include <mailutils/dbm.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>
#include <mailutils/locker.h>
#include "mudbm.h"

#ifndef O_EXCL
# define O_EXCL 0
#endif

#if defined(WITH_BDB)
#include <db.h>

struct bdb_file
{
  DB *db;
  DBC *dbc;
  mu_locker_t locker;
};

static int
_bdb_file_safety (mu_dbm_file_t db, int mode, uid_t owner)
{
  return mu_file_safety_check (db->db_name, mode, owner, NULL);
}

static int
_bdb_get_fd (mu_dbm_file_t db, int *pag, int *dir)
{
  struct bdb_file *bdb_file = db->db_descr;
  int rc = bdb_file->db->fd (bdb_file->db, pag);
  if (rc)
    {
      db->db_errno.n = rc;
      return MU_ERR_FAILURE;
    }
  if (dir)
    *dir = *pag;
  return 0;
}
  
static int
do_bdb_open (mu_dbm_file_t mdb, int flags, int mode)
{  
  struct bdb_file *bdb_file = mdb->db_descr;
  int f, rc, locker_flags;
  enum mu_locker_mode locker_mode;
  int tfd = -1;
  
  switch (flags)
    {
    case MU_STREAM_CREAT:
      f = DB_CREATE|DB_TRUNCATE;
      locker_mode = mu_lck_exc;
      break;
      
    case MU_STREAM_READ:
      f = DB_RDONLY;
      locker_mode = mu_lck_shr;
      break;
      
    case MU_STREAM_RDWR:
      f = DB_CREATE;
      locker_mode = mu_lck_exc;
      break;
      
    default:
      return EINVAL;
    }

#ifdef DB_FCNTL_LOCKING
  f |= DB_FCNTL_LOCKING;
  locker_flags = MU_LOCKER_KERNEL;
#else
  locker_flags = 0;
#endif
  rc = mu_locker_create (&bdb_file->locker, mdb->db_name,
			 locker_flags|MU_LOCKER_RETRY);
  if (rc)
    return rc;

  if (access (mdb->db_name, R_OK) && errno == ENOENT)
    {
      tfd = open (mdb->db_name, O_CREAT|O_RDONLY|O_EXCL,
                  mu_safety_criteria_to_file_mode (mdb->db_safety_flags));
      if (tfd == -1)
        {
          mu_locker_destroy (&bdb_file->locker);
          return errno;
        }
    }
  
  rc = mu_locker_lock_mode (bdb_file->locker, locker_mode);
  if (tfd != -1)
    close (tfd);
  switch (rc)
    {
    case 0:
      break;

    case EACCES:
      mu_locker_destroy (&bdb_file->locker);
      break;
      
    default:
      return rc;
    }
  
  rc = db_create (&bdb_file->db, NULL, 0);
  if (rc != 0 || bdb_file->db == NULL)
    return MU_ERR_FAILURE;
#if DB_VERSION_MAJOR == 3
  rc = bdb_file->db->open (bdb_file->db, mdb->db_name, NULL, DB_HASH, f, mode);
#else
  rc = bdb_file->db->open (bdb_file->db, NULL, mdb->db_name, NULL, DB_HASH,
			   f, mode);
#endif
  
  if (rc)
    return MU_ERR_FAILURE;

  return 0;
}

static int
_bdb_close (mu_dbm_file_t db)
{
  if (db->db_descr)
    {
      struct bdb_file *bdb_file = db->db_descr;
      if (bdb_file->db)
	bdb_file->db->close (bdb_file->db, 0);
      if (bdb_file->locker)
	{
	  mu_locker_unlock (bdb_file->locker);
	  mu_locker_destroy (&bdb_file->locker);
	}
      free (bdb_file);
      db->db_descr = NULL;
    }
  return 0;
}

static int
_bdb_open (mu_dbm_file_t mdb, int flags, int mode)
{
  int rc;
  struct bdb_file *bdb_file;

  bdb_file = calloc (1, sizeof *bdb_file);
  if (!bdb_file)
    return ENOMEM;

  mdb->db_descr = bdb_file;
  rc = do_bdb_open (mdb, flags, mode);
  if (rc)
    _bdb_close (mdb);
  return rc;
}

static int
_bdb_fetch (mu_dbm_file_t db, struct mu_dbm_datum const *key,
	    struct mu_dbm_datum *ret)
{
  DBT keydat, content;
  struct bdb_file *bdb_file = db->db_descr;
  int rc;

  memset (&keydat, 0, sizeof keydat);
  keydat.data = key->mu_dptr;
  keydat.size = key->mu_dsize;
  memset (&content, 0, sizeof content);
  content.flags = DB_DBT_MALLOC;
  rc = bdb_file->db->get (bdb_file->db, NULL, &keydat, &content, 0);
  mu_dbm_datum_free (ret);
  switch (rc)
    {
    case 0:
      ret->mu_dptr = content.data;
      ret->mu_dsize = content.size;
      ret->mu_sys = db->db_sys;
      break;

    case DB_NOTFOUND:
      db->db_errno.n = rc;
      rc = MU_ERR_NOENT;
      break;

    default:
      db->db_errno.n = rc;
      rc = MU_ERR_FAILURE;
    }
  return rc;
}

static int
_bdb_store (mu_dbm_file_t db,
	    struct mu_dbm_datum const *key,
	    struct mu_dbm_datum const *contents,
	    int replace)
{
  struct bdb_file *bdb_file = db->db_descr;
  int rc;
  DBT keydat, condat;

  memset (&keydat, 0, sizeof keydat);
  keydat.data = key->mu_dptr;
  keydat.size = key->mu_dsize;

  memset (&condat, 0, sizeof condat);
  condat.data = contents->mu_dptr;
  condat.size = contents->mu_dsize;

  rc = bdb_file->db->put (bdb_file->db, NULL, &keydat, &condat,
			  replace ? 0 : DB_NOOVERWRITE);
  db->db_errno.n = rc;
  switch (rc)
    {
    case 0:
      break;

    case DB_KEYEXIST:
      rc = MU_ERR_EXISTS;
      break;

    default:
      rc = MU_ERR_FAILURE;
      break;
    }
  return rc;
}

static int
_bdb_delete (mu_dbm_file_t db, struct mu_dbm_datum const *key)
{
  DBT keydat;
  struct bdb_file *bdb_file = db->db_descr;
  int rc;

  memset (&keydat, 0, sizeof keydat);
  keydat.data = key->mu_dptr;
  keydat.size = key->mu_dsize;
  rc = bdb_file->db->del (bdb_file->db, NULL, &keydat, 0);
  switch (rc)
    {
    case 0:
      break;

    case DB_NOTFOUND:
      db->db_errno.n = rc;
      rc = MU_ERR_NOENT;
      break;

    default:
      db->db_errno.n = rc;
      rc = MU_ERR_FAILURE;
    }
  return rc;
}

static int
_bdb_firstkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  struct bdb_file *bdb_file = db->db_descr;
  int rc;
  DBT key, dbt;

  if (!bdb_file->dbc)
    {
      rc = bdb_file->db->cursor (bdb_file->db, NULL, &bdb_file->dbc
				 BDB2_CURSOR_LASTARG);
      if (rc)
	{
	  db->db_errno.n = rc;
	  return MU_ERR_FAILURE;
	}
    }
  memset (&key, 0, sizeof key);
  key.flags = DB_DBT_MALLOC;
  
  memset (&dbt, 0, sizeof dbt);
  dbt.flags = DB_DBT_MALLOC;
  rc = bdb_file->dbc->c_get (bdb_file->dbc, &key, &dbt, DB_FIRST);
  mu_dbm_datum_free (ret);
  switch (rc)
    {
    case 0:
      free (dbt.data); /* FIXME: cache it for the eventual fetch that can
			  follow */
      ret->mu_dptr = key.data;
      ret->mu_dsize = key.size;
      ret->mu_sys = db->db_sys;
      break;

    case DB_NOTFOUND:
      db->db_errno.n = rc;
      rc = MU_ERR_NOENT;
      break;

    default:
      db->db_errno.n = rc;
      rc = MU_ERR_FAILURE;
    }
  return rc;
}

static int
_bdb_nextkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  struct bdb_file *bdb_file = db->db_descr;
  int rc;
  DBT key, dbt;

  if (!bdb_file->dbc)
    return MU_ERR_SEQ;

  memset (&key, 0, sizeof key);
  key.flags = DB_DBT_MALLOC;
  
  memset (&dbt, 0, sizeof dbt);
  dbt.flags = DB_DBT_MALLOC;
  rc = bdb_file->dbc->c_get (bdb_file->dbc, &key, &dbt, DB_NEXT);
  mu_dbm_datum_free (ret);
  switch (rc)
    {
    case 0:
      free (dbt.data); /* FIXME: cache it for the eventual fetch that can
			  follow */
      ret->mu_dptr = key.data;
      ret->mu_dsize = key.size;
      ret->mu_sys = db->db_sys;
      break;

    case DB_NOTFOUND:
      db->db_errno.n = rc;
      rc = MU_ERR_NOENT;
      break;

    default:
      db->db_errno.n = rc;
      rc = MU_ERR_FAILURE;
    }
  return rc;
}

static void
_bdb_datum_free (struct mu_dbm_datum *datum)
{
  free (datum->mu_dptr);
}

static char const *
_bdb_strerror (mu_dbm_file_t db)
{
  return db_strerror (db->db_errno.n);
}

struct mu_dbm_impl _mu_dbm_bdb = {
  "bdb",
  _bdb_file_safety,
  _bdb_get_fd,
  _bdb_open,
  _bdb_close,
  _bdb_fetch,
  _bdb_store,
  _bdb_delete,
  _bdb_firstkey,
  _bdb_nextkey,
  _bdb_datum_free,
  _bdb_strerror
};
#endif
