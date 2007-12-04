/* This file is part of GNU Mailutils
   Copyright (C) 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 3, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include "mailutils/libcfg.h"
#include <mailutils/libsieve.h>

static struct mu_gocs_sieve sieve_settings;

static int
cb_clear_library_path (mu_debug_t debug, void *data, char *arg)
{
  int flag;
  if (mu_cfg_parse_boolean (arg, &flag))
    {
      mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("not a boolean"));
      return 1;
    }
  if (flag)
    sieve_settings.clearflags |= MU_SIEVE_CLEAR_LIBRARY_PATH;
  return 0;
}

static int
cb_clear_include_path (mu_debug_t debug, void *data, char *arg)
{
  int flag;
  if (mu_cfg_parse_boolean (arg, &flag))
    {
      mu_cfg_format_error (debug, MU_DEBUG_ERROR, _("not a boolean"));
      return 1;
    }
  if (flag)
    sieve_settings.clearflags |= MU_SIEVE_CLEAR_INCLUDE_PATH;
  return 0;
}

static void
destroy_string (void *str)
{
  free (str);
}

static int
_add_path (mu_list_t *plist, mu_debug_t debug, void *data, char *arg)
{
  char *p;
  
  if (!*plist)
    {
      int rc = mu_list_create (plist);
      if (rc)
	{
	  mu_cfg_format_error (debug, MU_DEBUG_ERROR,
			       _("cannot create list: %s"), mu_strerror (rc));
	  exit (1);
	}
      mu_list_set_destroy_item (*plist, destroy_string);
    }
  for (p = strtok (arg, ":"); p; p = strtok (NULL, ":"))
    mu_list_append (*plist, strdup (p));
  return 0;
}

static int
cb_include_path (mu_debug_t debug, void *data, char *arg)
{
  return _add_path (&sieve_settings.include_path, debug, data, arg);
}

static int
cb_library_path (mu_debug_t debug, void *data, char *arg)
{
  return _add_path (&sieve_settings.library_path, debug, data, arg);
}

static struct mu_cfg_param mu_sieve_param[] = {
  { "clear-library-path", mu_cfg_callback, NULL, cb_clear_library_path,
     N_("Clear library search path.") },
  { "clear-include-path", mu_cfg_callback, NULL, cb_clear_include_path,
     N_("Clear include search path.") },
  { "library-path", mu_cfg_callback, NULL, cb_library_path,
    N_("Add directories to the library search path.  Argument is a "
       "comma-separated list of directories."),
    N_("list") },
  { "include-path", mu_cfg_callback, NULL, cb_include_path,
    N_("Add directories to the include search path.  Argument is a "
       "comma-separated list of directories."),
    N_("list") },
  { NULL }
};

DCL_CFG_CAPA (sieve);
