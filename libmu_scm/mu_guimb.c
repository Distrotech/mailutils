/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007 Free Software Foundation, Inc.

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

#include "mu_scm.h"

static void _scheme_main (void *closure, int argc, char **argv);

void
mu_process_mailbox (int argc, char *argv[], mu_guimb_param_t *param)
{
  scm_boot_guile (argc, argv, _scheme_main, param);
}

SCM _current_mailbox;
SCM _user_name;

static SCM
catch_body (void *closure)
{
  mu_guimb_param_t *param = closure;
  return param->catch_body (param->data, param->mbox);
}

void
_scheme_main (void *closure, int argc, char **argv)
{
  mu_guimb_param_t *param = closure;
  
  if (param->debug_guile)
    {
      SCM_DEVAL_P = 1;
      SCM_BACKTRACE_P = 1;
      SCM_RECORD_POSITIONS_P = 1;
      SCM_RESET_DEBUG_MODE;
    }

/* Initialize scheme library */
  mu_scm_init ();

  /* Provide basic primitives */
#include <mu_guimb.x>

  _current_mailbox = mu_scm_mailbox_create (param->mbox);
  mu_set_variable ("current-mailbox", _current_mailbox);

  _user_name = param->user_name ?
                       scm_makfrom0str (param->user_name) : SCM_BOOL_F;
  mu_set_variable ("user-name", _user_name);

  if (param->init) 
    param->init (param->data);
  
  do {
    scm_internal_lazy_catch (SCM_BOOL_T,
			     catch_body, closure,
			     param->catch_handler, param->data);
  } while (param->next && param->next (param->data, param->mbox));

  if (param->exit)
    exit (param->exit (param->data, param->mbox));

  exit (0);
}
