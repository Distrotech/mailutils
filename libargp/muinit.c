/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

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

#include "mailutils/libcfg.h"
#include "mailutils/libargp.h"

void
mu_argp_init (const char *vers, const char *bugaddr)
{
  argp_program_version = vers ? vers : PACKAGE_STRING;
  argp_program_bug_address = bugaddr ? bugaddr : "<" PACKAGE_BUGREPORT ">";
}

int
mu_app_init (struct argp *myargp, const char **capa,
	     struct mu_cfg_param *cfg_param,
	     int argc, char **argv, int flags, int *pindex, void *data)
{
  int rc, i;
  struct argp *argp;
  const struct argp argpnull = { 0 };
  
  mu_set_program_name (argv[0]);
  mu_libargp_init ();
  for (i = 0; capa[i]; i++)
    mu_gocs_register_std (capa[i]); /*FIXME*/
  if (!myargp)
    myargp = &argpnull;
  argp = mu_argp_build (myargp);
  rc = argp_parse (argp, argc, argv, flags, pindex, data);
  mu_argp_done (argp);
  if (rc)
    return rc;
  
  mu_libcfg_init (capa);
  mu_parse_config_files (cfg_param);

  mu_gocs_flush ();

  return 0;
}
