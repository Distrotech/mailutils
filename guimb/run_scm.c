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

void
_scheme_main ()
{
  SCM *scm_loc;

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

  if (program_file)
    scm_primitive_load (scm_makfrom0str (program_file));

  if (program_expr)
    scm_eval_0str (program_expr);

  collect_drop_mailbox ();

}
