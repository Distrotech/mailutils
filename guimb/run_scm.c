/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "guimb.h"
#include <mu_scm.h>

static void _scheme_main ();

void
run_main (int argc, char *argv[])
{
  /* Finish creating input mailbox */
  collect_create_mailbox ();
  scm_boot_guile (argc, argv, _scheme_main, NULL);
}

SCM _current_mailbox;
SCM _user_name;

static SCM
catch_body (void *data)
{
  if (program_file)
    scm_primitive_load (scm_makfrom0str (program_file));

  if (program_expr)
    scm_eval_0str (program_expr);

  return SCM_BOOL_F;
}

static SCM
catch_handler (void *data, SCM tag, SCM throw_args)
{
  collect_drop_mailbox ();
  return scm_handle_by_message ("guimb", tag, throw_args);
}

void
_scheme_main ()
{
  SCM *scm_loc;
  int rc;
  
  if (debug_guile)
    {
      SCM_DEVAL_P = 1;
      SCM_BACKTRACE_P = 1;
      SCM_RECORD_POSITIONS_P = 1;
      SCM_RESET_DEBUG_MODE;
    }

/* Initialize scheme library */
  mu_scm_init ();

  /* Provide basic primitives */
#include <run_scm.x>

  _current_mailbox = mu_scm_mailbox_create (mbox);
  scm_loc = SCM_CDRLOC (scm_sysintern ("current-mailbox", SCM_EOL));
  *scm_loc = _current_mailbox;

  _user_name = user_name ? scm_makfrom0str (user_name) : SCM_BOOL_F;
  scm_loc = SCM_CDRLOC (scm_sysintern ("user-name", SCM_EOL));
  *scm_loc = _user_name;

  scm_internal_lazy_catch (SCM_BOOL_T,
			   catch_body, NULL,
			   catch_handler, NULL);

  rc = collect_output ();
  collect_drop_mailbox ();
  exit (rc);
}
