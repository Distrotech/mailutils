/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2012 Free Software Foundation,
   Inc.

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

#ifdef HAVE_LIBLTDL
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>
#include <sieve-priv.h>
#include <ltdl.h>
#include <mailutils/cctype.h>

typedef int (*sieve_module_init_t) (mu_sieve_machine_t mach);

#if 0
/* FIXME: See comment below */ 
static void
_free_loaded_module (void *data)
{
  lt_dlclose ((lt_dlhandle)data);
  lt_dlexit ();
}
#endif

static int _add_load_dir (void *, void *);

static int
sieve_init_load_path ()
{
  static int inited = 0;

  if (!inited)
    {
      if (lt_dlinit ())
	return 1;
      mu_list_foreach (mu_sieve_library_path_prefix, _add_load_dir, NULL);
#ifdef MU_SIEVE_MODDIR
      _add_load_dir (MU_SIEVE_MODDIR, NULL);
#endif
      mu_list_foreach (mu_sieve_library_path, _add_load_dir, NULL);
      inited = 1;
    }
  return 0;
}
  
     
static lt_dlhandle
load_module (mu_sieve_machine_t mach, const char *name)
{
  lt_dlhandle handle;

  if (sieve_init_load_path ())
    return NULL;

  handle = lt_dlopenext (name);
  if (handle)
    {
      sieve_module_init_t init = (sieve_module_init_t)
	                                lt_dlsym (handle, "init");
      if (init)
	{
	  init (mach);
	  /* FIXME: We used to have this:
  	       mu_sieve_machine_add_destructor (mach, _free_loaded_module,
 	                                        handle);
             However, unloading modules can lead to random segfaults in
	     case they allocated any global-access data (e.g. mach->msg).
	     In particular, this was the case with extensions/pipe.c. 
	  */
	  return handle;
	}
      else
	{
	  lt_dlclose (handle);
	  handle = NULL;
	}
    }

  if (!handle)
    {
      mu_sieve_error (mach, "%s: %s", name, lt_dlerror ());
      lt_dlexit ();
    }
  return handle;
}

static void
fix_module_name (char *name)
{
  for (; *name; name++)
    {
      if (mu_isalnum (*name) || *name == '.' || *name == ',')
	continue;
      *name = '-';
    }
}

int
mu_sieve_load_ext (mu_sieve_machine_t mach, const char *name)
{
  lt_dlhandle handle;
  char *modname;

  modname = strdup (name);
  if (!modname)
    return 1;
  fix_module_name (modname);
  handle = load_module (mach, modname);
  free (modname);
  return handle == NULL;
}

static int
_add_load_dir (void *item, void *unused)
{
  return lt_dladdsearchdir (item);
}

int
mu_sv_load_add_dir (mu_sieve_machine_t mach, const char *name)
{
  if (sieve_init_load_path ())
    return 1;
  mu_sieve_machine_add_destructor (mach, (mu_sieve_destructor_t) lt_dlexit, 
                                   NULL);
  return lt_dladdsearchdir (name);
}

#else
#include <sieve-priv.h>

int
mu_sieve_load_ext (mu_sieve_machine_t mach, const char *name)
{
  return 1;
}

int
mu_sv_load_add_dir (mu_sieve_machine_t mach, const char *name)
{
  return 1;
}

#endif /* HAVE_LIBLTDL */
