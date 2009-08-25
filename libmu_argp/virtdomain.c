/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2009 Free Software Foundation, Inc.

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

#include "cmdline.h"

#define OPT_PWDDIR 256

static error_t
mu_virt_argp_parser (int key, char *arg, struct argp_state *state)
{
  static struct mu_argp_node_list lst;

  switch (key)
    {
    case OPT_PWDDIR:
      mu_argp_node_list_new (&lst, "passwd-dir", arg);
      break;

    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;

    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (&lst, "virtdomain", NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_option mu_virt_argp_option[] = {
  { "virtual-passwd-dir", OPT_PWDDIR, N_("DIR"), OPTION_HIDDEN,
    N_("search for virtual passwd file in DIR"), 0},
    { NULL,      0, NULL, 0, NULL, 0 }
};

static struct argp mu_virt_argp = {
  mu_virt_argp_option,
  mu_virt_argp_parser,
};

struct argp_child mu_virt_argp_child = {
  &mu_virt_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_virtdomain_cmdline = {
  "virtdomain", &mu_virt_argp_child
};
