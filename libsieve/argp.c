/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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
#include <sieve.h>
#include <mailutils/argp.h>
#include <string.h>

mu_list_t mu_sieve_include_path = NULL;
mu_list_t mu_sieve_library_path = NULL;

static error_t sieve_argp_parser (int key, char *arg, struct argp_state *state);

#define CLEARPATH_OPTION 256

/* Options used by programs that use extended authentication mechanisms. */
static struct argp_option sieve_argp_option[] = {
  { "includedir", 'I', N_("DIR"), 0,
    N_("Append directory DIR to the list of directories searched for include files"), 0 },
  { "libdir", 'L', N_("DIR"), 0,
    N_("Append directory DIR to the list of directories searched for library files"), 0 },
  { "clearpath", CLEARPATH_OPTION, NULL, 0,
    N_("Clear Sieve load path"), 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static struct argp sieve_argp = {
  sieve_argp_option,
  sieve_argp_parser,
};

static struct argp_child sieve_argp_child = {
  &sieve_argp,
  0,
  "Sieve options",
  0
};

static void
destroy_string (void *str)
{
  free (str);
}

static error_t
sieve_argp_parser (int key, char *arg, struct argp_state *state)
{
  mu_list_t *plist = NULL;
  
  switch (key)
    {
    case 'I':
      plist = &mu_sieve_include_path;
      break;

    case 'L':
      plist = &mu_sieve_library_path;
      break;

    case CLEARPATH_OPTION:
      mu_list_destroy (&mu_sieve_library_path);
      break;
      
    case ARGP_KEY_INIT:
#ifdef SIEVE_MODDIR
      plist = &mu_sieve_library_path;
      arg = SIEVE_MODDIR;
#endif
      break;
      
    case ARGP_KEY_FINI:
      sieve_load_add_path (mu_sieve_library_path);
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
	      argp_error (state, "can't create list: %s",
			  mu_strerror (rc));
	      exit (1);
	    }
	  mu_list_set_destroy_item (plist, destroy_string);
	}
      mu_list_append (*plist, strdup (arg));
    }
	  
  return 0;
}

void
mu_sieve_argp_init ()
{
  if (mu_register_capa ("sieve", &sieve_argp_child))
    {
      mu_error ("INTERNAL ERROR: cannot register argp capability sieve");
      abort ();
    }
}
