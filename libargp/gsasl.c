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
#include "cmdline.h"
#include <mailutils/gsasl.h>

#define OPT_CRAM_PASSWD 256

static struct argp_option _gsasl_argp_options[] = {
  {"cram-passwd", OPT_CRAM_PASSWD, N_("FILE"), OPTION_HIDDEN,
   N_("Specify password file for CRAM-MD5 authentication"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
_gsasl_argp_parser (int key, char *arg, struct argp_state *state)
{
  static struct mu_argp_node_list lst;

  switch (key)
    {
    case OPT_CRAM_PASSWD:
      mu_argp_node_list_new (&lst, "cram-passwd", arg);
      break;

    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;
      
    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (&lst, "gsasl", NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp _gsasl_argp = {
  _gsasl_argp_options,
  _gsasl_argp_parser
};

struct argp_child mu_gsasl_argp_child = {
  &_gsasl_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_gsasl_cmdline = {
  "gsasl", &mu_gsasl_argp_child
};


