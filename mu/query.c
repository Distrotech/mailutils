/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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

#include "mu.h"

char query_docstring[] = N_("query configuration values");
static char query_args_doc[] = N_("PATH [PATH...]");

static char *file_name;
int value_option;
int path_option;
int verbose_option;
char *progname;

static struct mu_option query_options[] = {
  { "file", 'f', N_("FILE"), MU_OPTION_DEFAULT,
    N_("query configuration values from FILE (default mailutils.rc)"),
    mu_c_string, &file_name },
  { "value", 0, NULL, MU_OPTION_DEFAULT,
    N_("display parameter values only"),
    mu_c_bool, &value_option },
  { "program", 'p', N_("NAME"), MU_OPTION_DEFAULT,
    N_("set program name for configuration lookup"),
    mu_c_string, &progname },
  { "path", 0, NULL, MU_OPTION_DEFAULT,
    N_("display parameters as paths"),
    mu_c_bool, &path_option },
  { "verbose", 'v', NULL, MU_OPTION_DEFAULT,
    N_("increase output verbosity"),
    mu_c_bool, &verbose_option },
  MU_OPTION_END
};

int
mutool_query (int argc, char **argv)
{
  static struct mu_cfg_parse_hints hints;
  mu_cfg_tree_t *tree = NULL;

  mu_action_getopt (&argc, &argv, query_options, query_docstring,
		    query_args_doc);

  if (argc == 0)
    {
      mu_error (_("query what?"));
      return 1;
    }

  hints.flags = MU_CFHINT_SITE_FILE;
  hints.site_file = file_name ? file_name : mu_site_config_file ();

  if (progname)
    {
      hints.flags |= MU_CFHINT_PROGRAM;
      hints.program = progname;
    }

  if (verbose_option)
    hints.flags |= MU_CF_FMT_LOCUS;
  if (value_option)
    hints.flags |= MU_CF_FMT_VALUE_ONLY;
  if (path_option)
    hints.flags |= MU_CF_FMT_PARAM_PATH;
  
  if (mu_cfg_parse_config (&tree, &hints))
    return 1;
  if (!tree)
    return 0;
  for ( ; argc > 0; argc--, argv++)
    {
      char *path = *argv;
      mu_cfg_node_t *node;

      if (mu_cfg_find_node (tree, path, &node) == 0)
	mu_cfg_format_node (mu_strout, node, hints.flags);
    }
  return 0;
}
 

/*
  MU Setup: query
  mu-handler: mutool_query
  mu-docstring: query_docstring
  End MU Setup:
*/
  
