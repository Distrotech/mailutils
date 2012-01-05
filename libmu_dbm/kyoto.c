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
#include <unistd.h>
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

#if defined(WITH_KYOTOCABINET)
#include <kclangc.h>

struct kc_descr
{
  KCDB *db;    /* db */
  KCCUR *cur;  /* cursor */
  int fd;
};

static int
_kc_file_safety (mu_dbm_file_t db, int mode, uid_t owner)
{
  return mu_file_safety_check (db->db_name, mode, owner, NULL);
}

int
_kc_get_fd (mu_dbm_file_t db, int *pag, int *dir)
{
  struct kc_descr *kcd = db->db_descr;
  if (kcd->fd == -1)
    {
      kcd->fd = open (db->db_name, O_RDONLY);
      if (kcd->fd == -1)
	return errno;
    }
  *pag = kcd->fd;
  if (dir)
    *dir = kcd->fd;
  return 0;
}

static int
_kc_open (mu_dbm_file_t db, int flags, int mode)
{
  struct kc_descr *kcd;
  int f, result;
  mode_t save_um;
  KCDB *kdb;

  switch (flags)
    {
    case MU_STREAM_CREAT:
      f = KCOWRITER | KCOCREATE;
      break;

    case MU_STREAM_READ:
      f = KCOREADER;
      break;

    case MU_STREAM_RDWR:
      f = KCOREADER | KCOWRITER;
      break;

    default:
      return EINVAL;
    }

  kdb = kcdbnew ();

  save_um = umask (0666 & ~mode);
  result = kcdbopen (kdb, db->db_name, f);
  umask (save_um);
  if (!result)
    {
      db->db_errno.n = kcdbecode (kdb);
      return MU_ERR_FAILURE;
    }

  kcd = calloc (1, sizeof (*kcd));
  kcd->db = kdb;
  kcd->cur = NULL;
  db->db_descr = kcd;
  kcd->fd = -1;
  return 0;
}

static int
_kc_close (mu_dbm_file_t db)
{
  if (db->db_descr)
    {
      struct kc_descr *kcd = db->db_descr;
      if (kcd->fd != -1)
	close (kcd->fd);
      kcdbclose (kcd->db);
      kcdbdel (kcd->db);
      free (kcd);
      db->db_descr = NULL;
    }
  return 0;
}

static int
_kc_fetch (mu_dbm_file_t db, struct mu_dbm_datum const *key,
	   struct mu_dbm_datum *ret)
{
  struct kc_descr *kcd = db->db_descr;
  void *ptr;
  size_t retsize;

  ptr = kcdbget (kcd->db, key->mu_dptr, key->mu_dsize, &retsize);
  if (ptr)
    {
      mu_dbm_datum_free (ret);
      ret->mu_dptr = ptr;
      ret->mu_dsize = retsize;
      return 0;
    }
  else if ((db->db_errno.n = kcdbecode (kcd->db)) == KCENOREC)
    return MU_ERR_NOENT;
  return MU_ERR_FAILURE;
}

static int
_kc_store (mu_dbm_file_t db,
	   struct mu_dbm_datum const *key,
	   struct mu_dbm_datum const *contents,
	   int replace)
{
  struct kc_descr *kcd = db->db_descr;
  int result;

  if (replace)
    result = kcdbreplace (kcd->db, key->mu_dptr, key->mu_dsize,
			  contents->mu_dptr, contents->mu_dsize);
  else
    result = kcdbadd (kcd->db, key->mu_dptr, key->mu_dsize,
		      contents->mu_dptr, contents->mu_dsize);
  if (result)
    return 0;
  db->db_errno.n = kcdbecode (kcd->db);
  if (db->db_errno.n == KCEDUPREC)
    return MU_ERR_EXISTS;
  return MU_ERR_FAILURE;
}

static int
_kc_delete (mu_dbm_file_t db, struct mu_dbm_datum const *key)
{
  struct kc_descr *kcd = db->db_descr;

  if (kcdbremove (kcd->db, key->mu_dptr, key->mu_dsize))
    return 0;
  db->db_errno.n = kcdbecode (kcd->db);
  if (db->db_errno.n == KCENOREC)
    return MU_ERR_NOENT;
  return MU_ERR_FAILURE;
}

static int
_kc_firstkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  struct kc_descr *kcd = db->db_descr;
  void *ptr;
  size_t retsize;

  kcd->cur = kcdbcursor (kcd->db);
  kccurjump (kcd->cur);

  ptr = kccurgetkey (kcd->cur, &retsize, 1);
  if (ptr)
    {
      mu_dbm_datum_free (ret);
      ret->mu_dptr = ptr;
      ret->mu_dsize = retsize;
      return 0;
    }
  else if ((db->db_errno.n = kcdbecode (kcd->db)) == KCENOREC)
    {
      kccurdel (kcd->cur);
      kcd->cur = NULL;
      return MU_ERR_NOENT;
    }
  return MU_ERR_FAILURE;
}

static int
_kc_nextkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  struct kc_descr *kcd = db->db_descr;
  void *ptr;
  size_t retsize;

  ptr = kccurgetkey (kcd->cur, &retsize, 1);
  if (ptr)
    {
      mu_dbm_datum_free (ret);
      ret->mu_dptr = ptr;
      ret->mu_dsize = retsize;
      return 0;
    }
  else if ((db->db_errno.n = kcdbecode (kcd->db)) == KCENOREC)
    {
      kccurdel (kcd->cur);
      kcd->cur = NULL;
      return MU_ERR_NOENT;
    }
  return MU_ERR_FAILURE;
}

static void
_kc_datum_free (struct mu_dbm_datum *datum)
{
  free (datum->mu_dptr);
}

static char const *
_kc_strerror (mu_dbm_file_t db)
{
  return kcecodename (db->db_errno.n);
}

struct mu_dbm_impl _mu_dbm_kyotocabinet = {
  "kc",
  _kc_file_safety,
  _kc_get_fd,
  _kc_open,
  _kc_close,
  _kc_fetch,
  _kc_store,
  _kc_delete,
  _kc_firstkey,
  _kc_nextkey,
  _kc_datum_free,
  _kc_strerror
};
#endif
