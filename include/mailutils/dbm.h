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

#ifndef _MAILUTILS_DBM_H
#define _MAILUTILS_DBM_H

#include <mailutils/types.h>

struct mu_dbm_datum
{
  char *mu_dptr;               /* Data pointer */
  size_t mu_dsize;             /* Data size */
  void *mu_data;               /* Implementation-dependent data */
  struct mu_dbm_impl *mu_sys;  /* Pointer to implementation */
};

struct mu_dbm_impl
{
  char *_dbm_name;
  int (*_dbm_file_safety) (mu_dbm_file_t db, int mode, uid_t owner);
  int (*_dbm_get_fd) (mu_dbm_file_t db, int *pag, int *dir);
  int (*_dbm_open) (mu_dbm_file_t db, int flags, int mode);
  int (*_dbm_close) (mu_dbm_file_t db);
  int (*_dbm_fetch) (mu_dbm_file_t db, struct mu_dbm_datum const *key,
		     struct mu_dbm_datum *ret);
  int (*_dbm_store) (mu_dbm_file_t db, struct mu_dbm_datum const *key,
		     struct mu_dbm_datum const *contents, int replace);
  int (*_dbm_delete) (mu_dbm_file_t db,
		      struct mu_dbm_datum const *key);
  int (*_dbm_firstkey) (mu_dbm_file_t db, struct mu_dbm_datum *ret);
  int (*_dbm_nextkey) (mu_dbm_file_t db, struct mu_dbm_datum *ret);
  void (*_dbm_datum_free) (struct mu_dbm_datum *datum);
  char const *(*_dbm_strerror) (mu_dbm_file_t db);
};

extern mu_url_t mu_dbm_hint;

void mu_dbm_init (void);
mu_url_t mu_dbm_get_hint (void);

int mu_dbm_register (struct mu_dbm_impl *impl);
int mu_dbm_create_from_url (mu_url_t url, mu_dbm_file_t *db, int defsafety);
int mu_dbm_create (char *name, mu_dbm_file_t *db, int defsafety);
int mu_dbm_close (mu_dbm_file_t db);
void mu_dbm_datum_free (struct mu_dbm_datum *datum);
int mu_dbm_delete (mu_dbm_file_t db, struct mu_dbm_datum const *key);
void mu_dbm_destroy (mu_dbm_file_t *pdb);
int mu_dbm_fetch (mu_dbm_file_t db, struct mu_dbm_datum const *key,
		  struct mu_dbm_datum *ret);
int mu_dbm_store (mu_dbm_file_t db, struct mu_dbm_datum const *key,
		  struct mu_dbm_datum const *contents, int replace);
int mu_dbm_firstkey (mu_dbm_file_t db, struct mu_dbm_datum *ret);
int mu_dbm_nextkey (mu_dbm_file_t db, struct mu_dbm_datum *ret);
int mu_dbm_open (mu_dbm_file_t db, int flags, int mode);
int mu_dbm_safety_get_owner (mu_dbm_file_t db, uid_t *uid);
int mu_dbm_safety_get_flags (mu_dbm_file_t db, int *flags);
int mu_dbm_safety_set_owner (mu_dbm_file_t db, uid_t uid);
int mu_dbm_safety_set_flags (mu_dbm_file_t db, int flags);
int mu_dbm_safety_check (mu_dbm_file_t db);
char const *mu_dbm_strerror (mu_dbm_file_t db);
int mu_dbm_get_fd (mu_dbm_file_t db, int *pag, int *dir);
int mu_dbm_get_name (mu_dbm_file_t db, const char **pname);

int mu_dbm_impl_iterator (mu_iterator_t *itr);

#endif
