/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/mailutils.h>
#include <mailutils/libcfg.h>
#include "argp.h"
#include "mu.h"

static char query_doc[] = N_("mu query - query configuration values.");
char query_docstring[] = N_("query configuration values");
static char query_args_doc[] = N_("path [path...]");

char *file_name;
int fmtflags = 0;

enum {
  VALUE_OPTION = 256,
  PATH_OPTION
};

static struct argp_option query_options[] = {
  { "file", 'f', N_("FILE"), 0,
    N_("query configuration values from FILE (default mailutils.rc)"),
    0 },
  { "value", VALUE_OPTION, NULL, 0,
    N_("display parameter values only"),
    0 },
  { "path", PATH_OPTION, NULL, 0,
    N_("display parameters as paths") },
  { "verbose", 'v', NULL, 0,
    N_("increase output verbosity"), 0},
  { NULL }
};

static error_t
query_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      file_name = arg;
      break;

    case 'v':
      fmtflags |= MU_CFG_FMT_LOCUS;
      break;

    case VALUE_OPTION:
      fmtflags |= MU_CFG_FMT_VALUE_ONLY;
      break;
      
    case PATH_OPTION:
      fmtflags |= MU_CFG_FMT_PARAM_PATH;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp query_argp = {
  query_options,
  query_parse_opt,
  query_args_doc,
  query_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_query (int argc, char **argv)
{
  int index;
  mu_cfg_tree_t *tree = NULL;
  
  if (argp_parse (&query_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

  if (argc == 0)
    {
      mu_error (_("query what?"));
      return 1;
    }

  if (file_name)
    {
      mu_load_site_rcfile = 0;
      mu_load_user_rcfile = 0;
      mu_load_rcfile = file_name;
    }
  
  if (mu_libcfg_parse_config (&tree))
    return 1;
  if (!tree)
    return 0;
  for ( ; argc > 0; argc--, argv++)
    {
      char *path = *argv;
      mu_cfg_node_t *node;

      if (mu_cfg_find_node (tree, path, &node) == 0)
	mu_cfg_format_node (mu_strout, node, fmtflags);
    }
  return 0;
}
 

/*
  MU Setup: query
  mu-handler: mutool_query
  mu-docstring: query_docstring
  End MU Setup:
*/
  
