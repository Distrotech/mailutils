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
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/list.h>
#include <mailutils/url.h>
#include <mailutils/dbm.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/nls.h>
#include <mailutils/mu_auth.h>
#include "mudbm.h"

static mu_list_t implist;
mu_url_t mu_dbm_hint;

static void
_implist_free (void *p)
{
  struct mu_dbm_impl *impl = p;

  free (impl->_dbm_name);
  free (impl);
}

static int
_implist_cmp (const void *a, const void *b)
{
  struct mu_dbm_impl const *ia = a;
  struct mu_dbm_impl const *ib = b;

  return strcmp (ia->_dbm_name, ib->_dbm_name);
}

void
mu_dbm_init ()
{
  int rc;

  if (implist)
    return;
  
  rc = mu_list_create (&implist);
  if (rc)
    {
      mu_error (_("cannot initialize DBM subsystem: %s"),
		mu_strerror (rc));
      abort ();
    }
  mu_list_set_destroy_item (implist, _implist_free);
  mu_list_set_comparator (implist, _implist_cmp);
  /* Add built-in entries */
#ifdef WITH_GDBM
  mu_dbm_register (&_mu_dbm_gdbm);
#endif
#ifdef WITH_BDB
  mu_dbm_register (&_mu_dbm_bdb);
#endif
#ifdef WITH_NDBM
  mu_dbm_register (&_mu_dbm_ndbm);
#endif
#ifdef WITH_TOKYOCABINET
  mu_dbm_register (&_mu_dbm_tokyocabinet);
#endif
#ifdef WITH_KYOTOCABINET
  mu_dbm_register (&_mu_dbm_kyotocabinet);
#endif
  if (!mu_dbm_hint)
    {
      struct mu_dbm_impl *impl;

      if ((rc = mu_list_get (implist, 0, (void**) &impl)) ||
	  (rc = mu_url_create_null (&mu_dbm_hint)) ||
	  (rc = mu_url_set_scheme (mu_dbm_hint, impl->_dbm_name)))
	{
	  mu_error (_("cannot initialize DBM hint: %s"),
		    mu_strerror (rc));
	  abort ();
	}
    }
}

mu_url_t
mu_dbm_get_hint ()
{
  mu_dbm_init ();
  return mu_dbm_hint;
}

int
mu_dbm_register (struct mu_dbm_impl *impl)
{
  int rc;
  struct mu_dbm_impl *ptr;

  mu_dbm_init ();
  ptr = calloc (1, sizeof (*ptr));
  if (!ptr)
    return ENOMEM;
  *ptr = *impl;
  ptr->_dbm_name = strdup (impl->_dbm_name);
  if (!ptr->_dbm_name)
    {
      free (ptr);
      return ENOMEM;
    }
  rc = mu_list_append (implist, ptr);
  if (rc)
    _implist_free (ptr);
  return rc;
}

int
mu_dbm_create_from_url (mu_url_t url, mu_dbm_file_t *db, int defsafety)
{
  mu_dbm_file_t p;
  int flags;
  int rc;
  const char *db_name;
  struct mu_dbm_impl impl_key;
  struct mu_dbm_impl *impl;
  struct mu_auth_data *auth;
  int safety_flags = 0;
  int safety_flags_set = 0;
  uid_t owner_uid = getuid ();

  mu_dbm_init ();
  
  mu_url_get_flags (url, &flags);
  if ((flags & (MU_URL_HOST | MU_URL_PATH)) == (MU_URL_HOST | MU_URL_PATH))
    return MU_ERR_URL_EXTRA_PARTS;
  if (flags & MU_URL_HOST)
    rc = mu_url_sget_host (url, &db_name);
  else
    rc = mu_url_sget_path (url, &db_name);
  if (rc)
    return rc;
  
  rc = mu_url_sget_scheme (url, (const char**)&impl_key._dbm_name);
  if (rc)
    return rc;
  
  rc = mu_list_locate (implist, (void *) &impl_key, (void **) &impl);
  if (rc)
    return rc;

  if (flags & MU_URL_PARAM)
    {
      size_t fvc, i;
      char **fvp;

      mu_url_sget_fvpairs (url, &fvc, &fvp);
      for (i = 0; i < fvc; i++)
	{
	  const char *name = fvp[i];
	  int negate = 0;
	  int val;

	  if (*name == '-')
	    {
	      if (!safety_flags_set)
		/* Imply default safety settings */
		safety_flags = defsafety;
	      negate = 1;
	      name++;
	    }
	  else if (*name == '+')
	    name++;
	  
	  safety_flags_set = 1;
	  if (strncmp (name, "owner", 5) == 0)
	    {
	      val = MU_FILE_SAFETY_OWNER_MISMATCH;
	      if (name[5] == '=')
		{
		  auth = mu_get_auth_by_name (name + 6);
		  if (auth)
		    {
		      owner_uid = auth->uid;
		      mu_auth_data_free (auth);
		    }
		  else
		    {
		      char *end;
		      unsigned long uid;

		      errno = 0;
		      uid = strtoul (name + 6, &end, 0);
		      if (*end || errno)
			return MU_ERR_NO_SUCH_USER;
		      owner_uid = uid;
		    }
		}
	      else if (name[5])
		return MU_ERR_URL_INVALID_PARAMETER;
	    }
	  else if (strcmp (name, "none") == 0)
	    {
	      safety_flags = negate ? MU_FILE_SAFETY_ALL : MU_FILE_SAFETY_NONE;
	      continue;
	    }
	  else if (strcmp (name, "all") == 0)
	    {
	      safety_flags = negate ? MU_FILE_SAFETY_NONE : MU_FILE_SAFETY_ALL;
	      continue;
	    }
	  else if (strcmp (name, "default") == 0)
	    {
	      val = defsafety;
	    }
	  else if (mu_file_safety_name_to_code (name, &val))
	    return MU_ERR_URL_INVALID_PARAMETER;

	  if (negate)
	    safety_flags &= ~val;
	  else
	    safety_flags |= val;
	}
    }

  p = calloc (1, sizeof (*p));
  if (!p)
    return ENOMEM;
  p->db_name = strdup (db_name);
  if (!p->db_name)
    {
      free (p);
      return ENOMEM;
    }

  p->db_safety_flags = safety_flags_set ? safety_flags : defsafety;
  p->db_owner = owner_uid;
  p->db_sys = impl;

  *db = p;
  return 0;
}

int
mu_dbm_impl_iterator (mu_iterator_t *itr)
{
  mu_dbm_init ();
  return mu_list_get_iterator (implist, itr);
}

