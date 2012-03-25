/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2008, 2010-2012 Free Software
   Foundation, Inc.

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
#include <sieve-priv.h>
#include <string.h>

mu_list_t mu_sieve_include_path = NULL;
mu_list_t mu_sieve_library_path = NULL;
mu_list_t mu_sieve_library_path_prefix = NULL;

mu_debug_handle_t mu_sieve_debug_handle;

void
mu_sieve_debug_init ()
{
  if (!mu_sieve_debug_handle)
    mu_sieve_debug_handle = mu_debug_register_category ("sieve");
}

static int
_path_append (void *item, void *data)
{
  mu_list_t *plist = data;
  char *p;
  if (!*plist)
    {
      int rc = mu_list_create (plist);
      if (rc)
	{
	  mu_error (_("cannot create list: %s"), mu_strerror (rc));
	  exit (1);
	}
      mu_list_set_destroy_item (*plist, mu_list_free_item);
    }
  p = strdup (item);
  if (!p)
    return ENOMEM;
  return mu_list_append (*plist, p);
}

int
mu_sieve_module_init (enum mu_gocs_op op, void *data)
{
  struct mu_gocs_sieve *p;
  if (!(op == mu_gocs_op_set && data))
    return 0;
  p = data;

  if (p->clearflags & MU_SIEVE_CLEAR_INCLUDE_PATH)
    mu_list_destroy (&mu_sieve_include_path);
  mu_list_foreach (p->include_path, _path_append, &mu_sieve_include_path);
  if (p->clearflags & MU_SIEVE_CLEAR_LIBRARY_PATH)
    {
      mu_list_destroy (&mu_sieve_library_path);
      mu_list_destroy (&mu_sieve_library_path_prefix);
    }
  mu_list_foreach (p->library_path_prefix, _path_append,
	      &mu_sieve_library_path_prefix);
  mu_list_foreach (p->library_path, _path_append, &mu_sieve_library_path);
  mu_list_destroy (&p->library_path);
  mu_list_destroy (&p->library_path_prefix);
  mu_list_destroy (&p->include_path);
  mu_sieve_debug_init ();
  return 0;
}
