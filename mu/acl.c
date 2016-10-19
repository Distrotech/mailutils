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

char acl_docstring[] = N_("test access control lists");
static char acl_args_doc[] = N_("ADDRESS [ADDRESS...]");

static char *input_file_name;
static struct mu_sockaddr *target_sa;
static mu_acl_t acl;
static const char *path = "acl";

static struct mu_option acl_options[] = {
  { "file", 'f', N_("FILE"), MU_OPTION_DEFAULT,
    N_("read ACLs from FILE"),
    mu_c_string, &input_file_name },
  { "path", 'p', N_("PATH"), MU_OPTION_DEFAULT,
    N_("path to the ACL in the configuration tree"),
    mu_c_string, &path },
  { NULL }
};


static struct mu_cfg_param acl_cfg_param[] = {
  { "acl", mu_cfg_section, &acl, 0, NULL, "access control list" },
  { NULL }
};

int
mutool_acl (int argc, char **argv)
{
  int rc;
  mu_acl_result_t result;
  mu_cfg_tree_t *tree = NULL, *temp_tree = NULL;
  mu_cfg_node_t *node;
  struct mu_cfg_parse_hints hints;

  mu_action_getopt (&argc, &argv, acl_options, acl_docstring, acl_args_doc);

  if (argc == 0)
    {
      mu_error (_("not enough arguments"));
      return 1;
    }

  memset (&hints, 0, sizeof (hints));
  hints.flags = MU_CFG_PARSE_CUSTOM_RCFILE;
  hints.custom_rcfile = input_file_name;

  mu_acl_cfg_init ();
  if (mu_cfg_parse_config (&tree, &hints))
    exit (EX_CONFIG);
  if (!tree)
    return 0;
  
  if (mu_cfg_find_node (tree, path, &node))
    {
      mu_error (_("cannot find node: %s"), path);
      return 1;
    }

  mu_cfg_tree_create (&temp_tree);
  mu_cfg_tree_add_node (temp_tree, node);
  rc = mu_cfg_tree_reduce (temp_tree, NULL, acl_cfg_param, NULL);
  if (rc)
    return 1;
  if (!acl)
    {
      mu_error (_("No ACL found in config"));
      return 1;
    }
  
  while (argc--)
    {
      const char *ap = *argv++;

      rc = mu_sockaddr_from_node (&target_sa, ap, NULL, NULL);
      if (rc)
	{
	  mu_error ("mu_sockaddr_from_node: %s", mu_strerror (rc));
	  exit (1);
	}

      mu_printf ("Testing %s:\n", ap);
      rc = mu_acl_check_sockaddr (acl, target_sa->addr, target_sa->addrlen,
				  &result);
      mu_sockaddr_free_list (target_sa);
      if (rc)
	{
	  mu_error ("mu_acl_check_sockaddr failed: %s", mu_strerror (rc));
	  return 1;
	}

      switch (result)
	{
	case mu_acl_result_undefined:
	  mu_printf ("%s: undefined\n", ap);
	  break;
      
	case mu_acl_result_accept:
	  mu_printf ("%s: accept\n", ap);
	  break;

	case mu_acl_result_deny:
	  mu_printf ("%s: deny\n", ap);
	  break;
	}
    }

  mu_cfg_destroy_tree (&tree);
  mu_cfg_destroy_tree (&temp_tree);

  return 0;
}

/*
  MU Setup: acl
  mu-handler: mutool_acl
  mu-docstring: acl_docstring
  End MU Setup:
*/
