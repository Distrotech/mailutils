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

enum {
  OPT_TLS = 256,         
  OPT_SSL_CERT,   
  OPT_SSL_KEY,    
  OPT_SSL_CAFILE
};
  
static struct argp_option _tls_argp_options[] = {
  {"ssl-cert", OPT_SSL_CERT, N_("FILE"), OPTION_HIDDEN,
   N_("Specify SSL certificate file"), 0},
  {"ssl-key", OPT_SSL_KEY, N_("FILE"), OPTION_HIDDEN,
   N_("Specify SSL certificate key"), },
  {"ssl-cafile", OPT_SSL_CAFILE, N_("FILE"), OPTION_HIDDEN,
   N_("Specify trusted CAs file"), 0},
  {"tls", OPT_TLS, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Enable TLS support") },
  {NULL, 0, NULL, 0, NULL, 0}
};

static error_t
_tls_argp_parser (int key, char *arg, struct argp_state *state)
{
  static struct mu_argp_node_list lst;
  
  switch (key)
    {
    case OPT_TLS:
      mu_argp_node_list_new (&lst, "enable", arg ? arg : "yes");
      break;
      
    case OPT_SSL_CERT:
      mu_argp_node_list_new (&lst, "ssl-cert", arg);
      break;

    case OPT_SSL_KEY:
      mu_argp_node_list_new (&lst, "ssl-key", arg);
      break;

    case OPT_SSL_CAFILE:
      mu_argp_node_list_new (&lst, "ssl-cafile", arg);
      break;

    case ARGP_KEY_INIT:
      mu_argp_node_list_init (&lst);
      break;

    case ARGP_KEY_FINI:
      mu_argp_node_list_finish (&lst, "tls", NULL);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp _tls_argp = {
  _tls_argp_options,
  _tls_argp_parser
};

static struct argp_child _tls_argp_child = {
  &_tls_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_tls_cmdline = {
  "tls", &_tls_argp_child
};
