/* This file is part of GNU Mailutils
   Copyright (C) 2007-2008, 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   GNU Mailutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include "mailutils/libcfg.h"
#include <mailutils/sieve.h>

static struct mu_gocs_sieve sieve_settings;

static int
cb_clear_library_path (void *data, mu_config_value_t *val)
{
  int flag;

  if (mu_cfg_assert_value_type (val, MU_CFG_STRING))
    return 1;
  if (mu_cfg_parse_boolean (val->v.string, &flag))
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
  if (mu_cfg_parse_boolean (val->v.string, &flag))
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

DCL_CFG_CAPA (sieve);
