/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2008, 2010-2012, 2014-2016 Free
   Software Foundation, Inc.

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
#include <sieve-priv.h>
#include <string.h>
#include <mailutils/cli.h>

mu_list_t mu_sieve_include_path = NULL;
mu_list_t mu_sieve_library_path = NULL;
mu_list_t mu_sieve_library_path_prefix = NULL;
mu_debug_handle_t mu_sieve_debug_handle;

//FIXME: provide definition (from gocs.h)
static struct mu_gocs_sieve sieve_settings;

void
mu_sieve_debug_init (void)
{
  if (!mu_sieve_debug_handle)
    mu_sieve_debug_handle = mu_debug_register_category ("sieve");
}

/*FIXME: REMOVE BEGIN */
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
/* FIXME: REMOVE END */

static int
cb_clear_library_path (void *data, mu_config_value_t *val)
{
  int flag;

  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_str_to_c (val->v.string, mu_c_bool, &flag, NULL))
    {
      mu_error (_("not a boolean"));
      return 1;
    }
  if (flag)
    sieve_settings.clearflags |= MU_SIEVE_CLEAR_LIBRARY_PATH;
  return 0;
}

static int
cb_clear_include_path (void *data, mu_config_value_t *val)
{
  int flag;
  
  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_str_to_c (val->v.string, mu_c_bool, &flag, NULL))
    {
      mu_error (_("not a boolean"));
      return 1;
    }
  if (flag)
    sieve_settings.clearflags |= MU_SIEVE_CLEAR_INCLUDE_PATH;
  return 0;
}

static int
_add_path (const char *arg, void *data)
{
  mu_list_t *plist = data;
    
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
  return mu_string_split (arg, ":", *plist);
}

static int
cb_include_path (void *data, mu_config_value_t *val)
{
  return mu_cfg_string_value_cb (val, _add_path,
				 &sieve_settings.include_path);
}  

static int
cb_library_path (void *data, mu_config_value_t *val)
{
  return mu_cfg_string_value_cb (val, _add_path,
				 &sieve_settings.library_path);
}

static int
cb_library_path_prefix (void *data, mu_config_value_t *val)
{
  return mu_cfg_string_value_cb (val, _add_path,
				 &sieve_settings.library_path_prefix);
}

static struct mu_cfg_param mu_sieve_param[] = {
  { "clear-library-path", mu_cfg_callback, NULL, 0, cb_clear_library_path,
     N_("Clear library search path.") },
  { "clear-include-path", mu_cfg_callback, NULL, 0, cb_clear_include_path,
     N_("Clear include search path.") },
  { "library-path", mu_cfg_callback, NULL, 0, cb_library_path,
    N_("Add directories to the library search path.  Argument is a "
       "colon-separated list of directories."),
    N_("list") },
  { "library-path-prefix", mu_cfg_callback, NULL, 0, cb_library_path_prefix,
    N_("Add directories to the beginning of the library search path.  "
       "Argument is a colon-separated list of directories."),
    N_("list") },
  { "include-path", mu_cfg_callback, NULL, 0, cb_include_path,
    N_("Add directories to the include search path.  Argument is a "
       "colon-separated list of directories."),
    N_("list") },
  { NULL }
};

/* New capability support */
static void
cli_includedir (struct mu_parseopt *po, struct mu_option *opt,
		char const *arg)
{
  _add_path (arg, &sieve_settings.include_path);
}

static void
cli_libdir  (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  _add_path (arg, &sieve_settings.library_path);
}

static void
cli_libdir_prefix (struct mu_parseopt *po, struct mu_option *opt,
		   char const *arg)
{
  _add_path (arg, &sieve_settings.library_path_prefix);
}

static void
cli_clear_include_path (struct mu_parseopt *po, struct mu_option *opt,
			char const *arg)
{
  sieve_settings.clearflags |= MU_SIEVE_CLEAR_INCLUDE_PATH;
}

static void
cli_clear_library_path (struct mu_parseopt *po, struct mu_option *opt,
			char const *arg)
{
  sieve_settings.clearflags |= MU_SIEVE_CLEAR_LIBRARY_PATH;
}

static struct mu_option sieve_option[] = {
  MU_OPTION_GROUP (N_("Sieve options")),
  { "includedir", 'I', N_("DIR"), MU_OPTION_DEFAULT,
    N_("append DIR to the list of directories searched for include files"),
    mu_c_string, NULL, cli_includedir },
  { "libdir", 'L', N_("DIR"), MU_OPTION_DEFAULT,
    N_("append DIR to the list of directories searched for library files"),
    mu_c_string, NULL, cli_libdir },
  { "libdir-prefix", 0, N_("DIR"), MU_OPTION_DEFAULT,
    N_("add DIR to the beginning of the list of directories searched for "
       "library files"),
    mu_c_string, NULL, cli_libdir_prefix },
  { "clear-include-path", 0, NULL, MU_OPTION_DEFAULT,
    N_("clear Sieve include path"),
    mu_c_string, NULL, cli_clear_include_path },
  { "clear-library-path", 0, NULL, MU_OPTION_DEFAULT,
    N_("clear Sieve library path"),
    mu_c_string, NULL, cli_clear_library_path },
  { "clearpath", 0, NULL, MU_OPTION_ALIAS },
  MU_OPTION_END
};

struct mu_cli_capa mu_cli_capa_sieve = {
  "sieve",
  sieve_option,
  mu_sieve_param,
  NULL, NULL
};
