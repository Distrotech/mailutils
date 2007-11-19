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
#include "mailutils/libargp.h"

static struct mu_cmdline_capa *all_cmdline_capa[] = {
  &mu_common_cmdline,
  &mu_logging_cmdline,
  &mu_license_cmdline,
  &mu_mailbox_cmdline,
  &mu_locking_cmdline,
  &mu_address_cmdline,
  &mu_mailer_cmdline,
  &mu_daemon_cmdline,
  &mu_pam_cmdline,
  &mu_gsasl_cmdline,
  &mu_tls_cmdline,
  &mu_radius_cmdline,
  &mu_sql_cmdline,
  &mu_virtdomain_cmdline,
  &mu_auth_cmdline,
  &mu_sieve_cmdline,
  NULL
};

static int libargp_init_passed = 0;

void
mu_libargp_init ()
{
  struct mu_cmdline_capa **cpp;
  if (libargp_init_passed)
    return;
  libargp_init_passed = 1;
  for (cpp = all_cmdline_capa; *cpp; cpp++)
    {
      struct mu_cmdline_capa *cp = *cpp;
      if (mu_register_argp_capa (cp->name, cp->child))
	{
	  mu_error (_("INTERNAL ERROR: cannot register argp capability `%s'"),
		    cp->name);
	  abort ();
	}
    }
}

  
  
