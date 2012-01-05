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
#include <mailutils/types.h>
#include <mailutils/dbm.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>
#include "mudbm.h"

#if defined(WITH_GDBM)
#include <gdbm.h>

struct gdbm_descr
{
  GDBM_FILE file;           /* GDBM file */
  datum prev;               /* Previous key for sequential access */
};

static int
_gdbm_file_safety (mu_dbm_file_t db, int mode, uid_t owner)
{
  return mu_file_safety_check (db->db_name, mode, owner, NULL);
}

int
_gdbm_get_fd (mu_dbm_file_t db, int *pag, int *dir)
{
  struct gdbm_descr *gd = db->db_descr;
  *pag = gdbm_fdesc (gd->file);
  if (dir)
    *dir = *pag;
  return 0;
}

static int
_gdbm_open (mu_dbm_file_t db, int flags, int mode)
{
  int f;
  struct gdbm_descr *gd;
  GDBM_FILE file;
  
  switch (flags)
    {
    case MU_STREAM_CREAT:
      f = GDBM_NEWDB;
      break;
      
    case MU_STREAM_READ:
      f = GDBM_READER;
      break;
      
    case MU_STREAM_RDWR:
      f = GDBM_WRCREAT;
      break;
      
    default:
      return EINVAL;
    }
  file = gdbm_open (db->db_name, 512, f, mode, NULL);
  if (!file)
    return MU_ERR_FAILURE;
  gd = calloc (1, sizeof (*gd));
  gd->file = file;
  db->db_descr = gd;
  return 0;
}

static int
_gdbm_close (mu_dbm_file_t db)
{
  if (db->db_descr)
    {
      struct gdbm_descr *gd = db->db_descr;
      gdbm_close (gd->file);
      free (gd);
      db->db_descr = NULL;
    }
  return 0;
}

static int
_gdbm_conv_datum (mu_dbm_file_t db, struct mu_dbm_datum *ret, datum content,
		  int copy)
{
  if (copy)
    {
      ret->mu_dptr = malloc (content.dsize);
      if (!ret->mu_dptr)
	return errno;
      memcpy (ret->mu_dptr, content.dptr, content.dsize);
    }
  else
    {
      ret->mu_dptr = content.dptr;
    }
  ret->mu_dsize = content.dsize;
  ret->mu_sys = db->db_sys;
  return 0;
}

static int
_gdbm_fetch (mu_dbm_file_t db, struct mu_dbm_datum const *key,
	     struct mu_dbm_datum *ret)
{
  struct gdbm_descr *gd = db->db_descr;
  int rc;
  datum keydat, content;

  keydat.dptr = key->mu_dptr;
  keydat.dsize = key->mu_dsize;
  gdbm_errno = 0;
  content = gdbm_fetch (gd->file, keydat);
  if (content.dptr == NULL)
    {
      if (gdbm_errno == GDBM_ITEM_NOT_FOUND)
	return MU_ERR_NOENT;
      else
	{
	  db->db_errno.n = gdbm_errno;
	  return MU_ERR_FAILURE;
	}
    }
  mu_dbm_datum_free (ret);
  rc = _gdbm_conv_datum (db, ret, content, 1);
  if (rc)
    {
      free (content.dptr);
      return rc;
    }
  return 0;
}

static int
_gdbm_store (mu_dbm_file_t db,
	     struct mu_dbm_datum const *key,
	     struct mu_dbm_datum const *contents,
	     int replace)
{
  struct gdbm_descr *gd = db->db_descr;
  datum keydat, condat;

  keydat.dptr = key->mu_dptr;
  keydat.dsize = key->mu_dsize;
  condat.dptr = contents->mu_dptr;
  condat.dsize = contents->mu_dsize;
  switch (gdbm_store (gd->file, keydat, condat, replace))
    {
    case 0:
      break;
      
    case 1:
      return MU_ERR_EXISTS;
      
    case -1:
      db->db_errno.n = gdbm_errno;
      return MU_ERR_FAILURE;
    }
  return 0;
}

static int
_gdbm_delete (mu_dbm_file_t db, struct mu_dbm_datum const *key)
{
  struct gdbm_descr *gd = db->db_descr;
  datum keydat;

  keydat.dptr = key->mu_dptr;
  keydat.dsize = key->mu_dsize;
  gdbm_errno = 0;
  if (gdbm_delete (gd->file, keydat))
    {
      if (gdbm_errno == GDBM_ITEM_NOT_FOUND)
	return MU_ERR_NOENT;
      else
	{
	  db->db_errno.n = gdbm_errno;
	  return MU_ERR_FAILURE;
	}
    }
  return 0;
}

static int
_gdbm_firstkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  struct gdbm_descr *gd = db->db_descr;
  int rc;
  datum key = gdbm_firstkey (gd->file);
  if (key.dptr == NULL)
    {
      if (gdbm_errno == GDBM_ITEM_NOT_FOUND)
	return MU_ERR_NOENT;
      else
	{
	  db->db_errno.n = gdbm_errno;
	  return MU_ERR_FAILURE;
	}
    }
  mu_dbm_datum_free (ret);
  rc = _gdbm_conv_datum (db, ret, key, 0);
  if (rc)
    {
      free (key.dptr);
      return rc;
    }
  free (gd->prev.dptr);
  gd->prev = key;
  return 0;
}

static int
_gdbm_nextkey (mu_dbm_file_t db, struct mu_dbm_datum *ret)
{
  struct gdbm_descr *gd = db->db_descr;
  int rc;
  datum key;

  if (!gd->prev.dptr)
    return MU_ERR_NOENT;
  key = gdbm_nextkey (gd->file, gd->prev);
  if (key.dptr == NULL)
    {
      if (gdbm_errno == GDBM_ITEM_NOT_FOUND)
	return MU_ERR_NOENT;
      else
	{
	  db->db_errno.n = gdbm_errno;
	  return MU_ERR_FAILURE;
	}
    }
  mu_dbm_datum_free (ret);
  rc = _gdbm_conv_datum (db, ret, key, 0);
  if (rc)
    {
      free (key.dptr);
      return rc;
    }
  free (gd->prev.dptr);
  gd->prev = key;
  return 0;
}

static void
_gdbm_datum_free (struct mu_dbm_datum *datum)
{
  free (datum->mu_dptr);
}

static char const *
_gdbm_strerror (mu_dbm_file_t db)
{
  return gdbm_strerror (db->db_errno.n);
}

struct mu_dbm_impl _mu_dbm_gdbm = {
  "gdbm",
  _gdbm_file_safety,
  _gdbm_get_fd,
  _gdbm_open,
  _gdbm_close,
  _gdbm_fetch,
  _gdbm_store,
  _gdbm_delete,
  _gdbm_firstkey,
  _gdbm_nextkey,
  _gdbm_datum_free,
  _gdbm_strerror
};
#endif
