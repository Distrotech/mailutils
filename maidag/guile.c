/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "maidag.h"

#ifdef WITH_GUILE
#include <mailutils/guile.h>

int debug_guile;
static int initialized;

int
scheme_check_msg (mu_message_t msg, struct mu_auth_data *auth,
		  const char *prog)
{
  if (!initialized)
    {
      mu_guile_init (debug_guile);
      if (mu_log_syslog)
	{
	  SCM port;
	  
	  port = mu_scm_make_debug_port (MU_DIAG_ERROR);
	  scm_set_current_error_port (port);
	  port = mu_scm_make_debug_port (MU_DIAG_INFO);
	  scm_set_current_output_port (port);
	}
      initialized = 1;
    }
  mu_guile_load (prog, 0, NULL);
  mu_guile_message_apply (msg, "mailutils-check-message");
  return 0;
}

#endif

