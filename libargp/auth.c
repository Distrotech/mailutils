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
#include "mailutils/mu_auth.h"


/* ************************************************************************* */
/* Traditional configuration                                                 */
/* ************************************************************************* */

enum {
  OPT_AUTHORIZATION=256,
  OPT_AUTHENTICATION,
  OPT_CLEAR_AUTHORIZATION,
  OPT_CLEAR_AUTHENTICATION,
  OPT_DEBUG_AUTH
};

static error_t mu_auth_argp_parser (int key, char *arg,
				    struct argp_state *state);

/* Options used by programs that use extended authentication mechanisms. */
static struct argp_option mu_auth_argp_option[] = {
  { "authentication", OPT_AUTHENTICATION, N_("MODLIST"), OPTION_HIDDEN,
    N_("Set the list of modules to be used for authentication"), 0 },
  { "authorization", OPT_AUTHORIZATION, N_("MODLIST"), OPTION_HIDDEN,
    N_("Set list of modules to be used for authorization"), 0 },
  { "clear-authorization", OPT_CLEAR_AUTHORIZATION, NULL, OPTION_HIDDEN,
    N_("Clear the list of authorization modules"), 0 },
  { "clear-authentication", OPT_CLEAR_AUTHENTICATION, NULL, OPTION_HIDDEN,
    N_("Clear the list of authentication modules"), 0 },
  { "debug-auth", OPT_DEBUG_AUTH, NULL, 0,
    N_("Debug authentication functions") },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static struct argp mu_auth_argp = {
  mu_auth_argp_option,
  mu_auth_argp_parser,
};

static struct argp_child mu_auth_argp_child = {
  &mu_auth_argp,
  0,
  NULL,
  0
};

static void
auth_set_debug ()
{
  mu_debug_t debug = NULL, prev;
  
  mu_debug_create (&debug, NULL);
  mu_debug_set_level (debug, MU_DEBUG_TRACE);
  prev = mu_auth_set_debug (debug);
  if (prev)
    mu_debug_destroy (&prev, mu_debug_get_owner (prev));
}

static error_t
mu_auth_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_FINI:
      mu_auth_finish_setup ();
      break;

      /* authentication */
    case OPT_AUTHORIZATION:
      mu_authorization_add_module_list (arg);
      break;
      
    case OPT_AUTHENTICATION:
      mu_authentication_add_module_list (arg);
      break;
      
    case OPT_CLEAR_AUTHENTICATION:
      mu_authentication_clear_list ();
      break;
      
    case OPT_CLEAR_AUTHORIZATION:
      mu_authorization_clear_list ();
      break;

    case OPT_DEBUG_AUTH:
      auth_set_debug ();
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct mu_cmdline_capa mu_auth_cmdline = {
  "auth", &mu_auth_argp_child
};
