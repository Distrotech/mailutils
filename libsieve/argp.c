/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <sieve.h>
#include <mailutils/argp.h>
#include <string.h>

list_t sieve_include_path = NULL;
list_t sieve_library_path = NULL;

static error_t sieve_argp_parser __P((int key, char *arg,
					struct argp_state *state));

/* Options used by programs that use extended authentication mechanisms. */
static struct argp_option sieve_argp_option[] = {
  { "includedir", 'I', N_("DIR"), 0,
    "Append directory DIR to the list of directories searched for include files", 0 },
  { "libdir", 'L', N_("DIR"), 0,
    "Append directory DIR to the list of directories searched for library files", 0 },
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

static error_t
sieve_argp_parser (int key, char *arg, struct argp_state *state)
{
  list_t *plist = NULL;
  
  switch (key)
    {
    case 'I':
      plist = &sieve_include_path;
      break;

    case 'L':
      plist = &sieve_library_path;
      break;

    case ARGP_KEY_INIT:
#ifdef SIEVE_MODDIR
      plist = &sieve_library_path;
      arg = SIEVE_MODDIR;
#endif
      break;
      
    case ARGP_KEY_FINI:
      sieve_load_add_path (sieve_library_path);
      break;
			   
    default:
      return ARGP_ERR_UNKNOWN;
    }

  if (plist)
    {
      if (!*plist)
	{
	  int rc = list_create (plist);
	  if (rc)
	    {
	      argp_error (state, "can't create list: %s",
			mu_strerror (rc));
	      exit (1);
	    }
	}
      list_append (*plist, strdup (arg));
    }
	  
  return 0;
}

void
sieve_argp_init ()
{
  if (mu_register_capa ("sieve", &sieve_argp_child))
    {
      mu_error ("INTERNAL ERROR: cannot register argp capability sieve");
      abort ();
    }
}
