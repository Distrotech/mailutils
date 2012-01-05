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
#include <unistd.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/list.h>
#include <mailutils/url.h>
#include <mailutils/dbm.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>
#include "mudbm.h"

int
mu_dbm_create (char *name, mu_dbm_file_t *db, int defsafety)
{
  int rc;
  mu_url_t url;

  mu_dbm_init ();
  rc = mu_url_create_hint (&url, name, 0, mu_dbm_hint);
  if (rc)
    return rc;
  rc = mu_dbm_create_from_url (url, db, defsafety);
  mu_url_destroy (&url);
  return rc;
}
