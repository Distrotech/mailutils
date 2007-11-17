/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005,
   2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include "cmdline.h"
#include "mailutils/libsieve.h"


static struct argp_option sieve_argp_option[] = {
  { "includedir", 'I', N_("DIR"), 0,
    N_("Append directory DIR to the list of directories searched for include files"), 0 },
  { "libdir", 'L', N_("DIR"), 0,
    N_("Append directory DIR to the list of directories searched for library files"), 0 },
  { "clear-include-path", OPT_CLEAR_INCLUDE_PATH, NULL, 0,
    N_("Clear Sieve include path"), 0 },
  { "clear-library-path", OPT_CLEAR_LIBRARY_PATH, NULL, 0,
    N_("Clear Sieve lobrary path"), 0 },
  { "clearpath", 0, NULL, OPTION_ALIAS, NULL },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
sieve_argp_parser (int key, char *arg, struct argp_state *state)
{
  static struct mu_gocs_sieve gocs_data;
  mu_list_t *plist = NULL;
  
  switch (key)
    {
    case 'I':
      plist = &gocs_data.include_path;
      break;

    case 'L':
      plist = &gocs_data.library_path;
      break;

    case OPT_CLEAR_INCLUDE_PATH:
      gocs_data.clearflags |= MU_SIEVE_CLEAR_INCLUDE_PATH;
      break;

    case OPT_CLEAR_LIBRARY_PATH:
      gocs_data.clearflags |= MU_SIEVE_CLEAR_LIBRARY_PATH;
      break;
      
    case ARGP_KEY_INIT:
      memset (&gocs_data, 0, sizeof gocs_data);
      break;
      
    case ARGP_KEY_FINI:
      mu_gocs_store ("sieve", &gocs_data);
      break;
			   
    default:
      return ARGP_ERR_UNKNOWN;
    }

  if (plist)
    {
      if (!*plist)
	{
	  int rc = mu_list_create (plist);
	  if (rc)
	    {
	      argp_error (state, _("cannot create list: %s"),
			  mu_strerror (rc));
	      exit (1);
	    }
	}
      mu_list_append (*plist, arg);
    }
	  
  return 0;
}

static struct argp sieve_argp = {
  sieve_argp_option,
  sieve_argp_parser,
};

static struct argp_child sieve_argp_child = {
  &sieve_argp,
  0,
  N_("Sieve options"),
  0
};

struct mu_cmdline_capa mu_sieve_cmdline = {
  "sieve", &sieve_argp_child
};

