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
#include <mailutils/radius.h>

static struct argp_option mu_radius_argp_option[] = {
  { "radius-auth-request", OPT_AUTH_REQUEST, N_("REQUEST"), OPTION_HIDDEN,
    N_("Radius request to authenitcate the user"), 0 },
  { "radius-getpwnam-request", OPT_GETPWNAM_REQUEST, N_("REQUEST"), OPTION_HIDDEN,
    N_("Radius request to retrieve a passwd entry based on username"), 0 },
  { "radius-getpwuid-request", OPT_GETPWUID_REQUEST, N_("REQUEST"), OPTION_HIDDEN,
    N_("Radius request to retrieve a passwd entry based on UID"), 0 },
  { "radius-directory", OPT_RADIUS_DIR, N_("DIR"), OPTION_HIDDEN,
    N_("Set path to the radius configuration directory"), 0 },
  { NULL }
};


static struct mu_radius_module_data radius_data;

static error_t
mu_radius_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case OPT_AUTH_REQUEST:
      radius_data.auth_request = arg;
      break;
      
    case OPT_GETPWNAM_REQUEST:
      radius_data.getpwnam_request = arg;
      break;
      
    case OPT_GETPWUID_REQUEST:
      radius_data.getpwuid_request = arg;
      break;
      
    case OPT_RADIUS_DIR:
      radius_data.config_dir = arg;
      break;
      
    case ARGP_KEY_FINI:
      mu_gocs_store ("radius", &radius_data); 
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp mu_radius_argp = {
  mu_radius_argp_option,
  mu_radius_argp_parser,
};

struct argp_child mu_radius_argp_child = {
  &mu_radius_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_radius_cmdline = {
  "radius", &mu_radius_argp_child
};