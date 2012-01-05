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
#include <sys/stat.h>
#include <mailutils/types.h>
#include <mailutils/dbm.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>
#include <mailutils/io.h>
#include "mudbm.h"

#if defined(WITH_TOKYOCABINET)
#include <tcutil.h>
#include <tchdb.h>

static int
_tc_file_safety (mu_dbm_file_t db, int mode, uid_t owner)
{
  return mu_file_safety_check (db->db_name, mode, owner, NULL);
}

int
_tc_get_fd (mu_dbm_file_t db, int *pag, int *dir)
{
  TCHDB *hdb = db->db_descr;
  *pag = hdb->fd;
  if (dir)
    *dir = *pag;
  return 0;
}

static int
_tc_open (mu_dbm_file_t db, int flags, int mode)
{
  int f;
  bool result;
  mode_t save_um;
  TCHDB *hdb;

  switch (flags)
    {
    case MU_STREAM_CREAT:
      f = HDBOWRITER | HDBOCREAT;
      break;

    case MU_STREAM_READ:
      f = HDBOREADER;
      break;

    case MU_STREAM_RDWR:
      f = HDBOREADER | HDBOWRITER;
      break;

    default:
      return EINVAL;
    }

  hdb = tchdbnew ();

  save_um = umask (0666 & ~mode);
  result = tchdbopen (hdb, db->db_name, f);
  umask (save_um);
  if (!result)
    {
      db->db_errno.n = tchdbecode (hdb);
      return MU_ERR_FAILURE;
    }
  db->db_descr = hdb;
  return 0;
}

static int
_tc_close (mu_dbm_file_t db)
{
  if (db->db_descr)
    {
      TCHDB *hdb = db->db_descr;
      tchdbclose (hdb);
      tchdbdel (hdb);
      db->db_descr = NULL;
    }
  return 0;
}

static int
_tc_fetch (mu_dbm_file_t db, struct mu_dbm_datum const *key,
	   struct mu_dbm_datum *ret)
{
  TCHDB *hdb = db->db_descr;
  void *ptr;
  int retsize;

  ptr = tchdbget (hdb, key->mu_dptr, key->mu_dsize, &retsize);
  if (ptr)
    {
      mu_dbm_datum_free (ret);
      ret->mu_dptr = ptr;
      ret->mu_dsize = retsize;
      return 0;
    }
  else if ((db->db_errno.n = tchdbecode (hdb)) == TCENOREC)
    return MU_ERR_NOENT;
  return MU_ERR_FAILURE;
}

static int
_tc_store (mu_dbm_file_t db,
	   struct mu_dbm_datum const *key,
	   struct mu_dbm_datum const *contents,
	   int replace)
{
  TCHDB *hdb = db->db_descr;
  bool result;

  if (replace)
    result = tchdbput (hdb, key->mu_dptr, key->mu_dsize,
		       contents->mu_dptr, contents->mu_dsize);
  else
    result = tchdbputkeep (hdb, key->mu_dptr, key->mu_dsize,
			   contents->mu_dptr, contents->mu_dsize);
  if (result)
    return 0;
  db->db_errno.n = tchdbecode (hdb);
  if (db->db_errno.n == TCEKEEP)
    return MU_ERR_EXISTS;
  return MU_ERR_FAILURE;
}

static int
_tc_delete (mu_dbm_file_t db, struct mu_dbm_datum const *key)
{
  TCHDB *hdb = db->db_descr;

  if (tchdbout (hdb, key->mu_dptr, key->mu_dsize))
    return 0;
  db->db_errno.n = tchdbecode (hdb);
  if (db->db_errno.n == TCENOREC)
    return MU_ERR_NOENT;
  return MU_ERR_FAILURE;
}

static int
_tc_firstkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  TCHDB *hdb = db->db_descr;
  void *ptr;
  int retsize;

  tchdbiterinit (hdb);
  ptr = tchdbiternext (hdb, &retsize);
  if (ptr)
    {
      mu_dbm_datum_free (ret);
      ret->mu_dptr = ptr;
      ret->mu_dsize = retsize;
      return 0;
    }
  else if ((db->db_errno.n = tchdbecode (hdb)) == TCENOREC)
    return MU_ERR_NOENT;
  return MU_ERR_FAILURE;
}

static int
_tc_nextkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  TCHDB *hdb = db->db_descr;
  void *ptr;
  int retsize;

  ptr = tchdbiternext (hdb, &retsize);
  if (ptr)
    {
      mu_dbm_datum_free (ret);
      ret->mu_dptr = ptr;
      ret->mu_dsize = retsize;
      return 0;
    }
  else if ((db->db_errno.n = tchdbecode (hdb)) == TCENOREC)
    return MU_ERR_NOENT;
  return MU_ERR_FAILURE;
}

static void
_tc_datum_free (struct mu_dbm_datum *datum)
{
  free (datum->mu_dptr);
}

static char const *
_tc_strerror (mu_dbm_file_t db)
{
  return tchdberrmsg (db->db_errno.n);
}

struct mu_dbm_impl _mu_dbm_tokyocabinet = {
  "tc",
  _tc_file_safety,
  _tc_get_fd,
  _tc_open,
  _tc_close,
  _tc_fetch,
  _tc_store,
  _tc_delete,
  _tc_firstkey,
  _tc_nextkey,
  _tc_datum_free,
  _tc_strerror
};
#endif
