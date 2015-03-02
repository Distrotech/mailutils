/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012, 2014-2015 Free
   Software Foundation, Inc.

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

#include "muscript.h"
#include "muscript_priv.h"
#include <mailutils/guile.h>

static int initialized;

static int
scheme_init (const char *prog, mu_script_descr_t *pdescr)
{
  if (!initialized)
    {
      mu_guile_init (mu_script_debug_guile);
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
  return 0;
}

static int
scheme_proc (mu_script_descr_t descr, mu_message_t msg)
{
  mu_guile_message_apply (msg, "mailutils-check-message");
  return 0;
}

struct mu_script_fun mu_script_scheme = {
  "scheme",
  "scm\0",
  scheme_init,
  NULL,
  scheme_proc,
  NULL
};
