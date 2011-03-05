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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <mailutils/mailutils.h>
#include <mailutils/libcfg.h>
#include "argp.h"
#include "mu.h"
#include "xalloc.h"

static char acl_doc[] = N_("mu acl - test access control lists.");
char acl_docstring[] = N_("test access control lists");
static char acl_args_doc[] = N_("ADDRESS [ADDRESS...]");

static struct argp_option acl_options[] = {
  { "file", 'f', N_("FILE"), 0, N_("read ACLs from FILE") },
  { "path", 'p', N_("PATH"), 0,
    N_("path to the ACL in the configuration tree") },
  { NULL }
};

static char *input_file_name;
static struct sockaddr *target_sa;
static socklen_t target_salen;
static mu_acl_t acl;
static const char *path = "acl";

static struct sockaddr *
parse_address (socklen_t *psalen, const char *str)
{
  struct sockaddr_in in;
  struct sockaddr *sa;
  
  in.sin_family = AF_INET;
  if (inet_aton (str, &in.sin_addr) == 0)
    {
      mu_error ("Invalid IPv4: %s", str);
      exit (1);
    }
  in.sin_port = 0;
  *psalen = sizeof (in);
  sa = malloc (*psalen);
  if (!sa)
    {
      mu_error ("%s", mu_strerror (errno));
      exit (1);
    }
  
  memcpy (sa, &in, sizeof (in));
  return sa;
}

static error_t
acl_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      input_file_name = arg;
      break;

    case 'p':
      path = arg;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp acl_argp = {
  acl_options,
  acl_parse_opt,
  acl_args_doc,
  acl_doc,
  NULL,
  NULL,
  NULL
};


static struct mu_cfg_param acl_cfg_param[] = {
  { "acl", mu_cfg_section, &acl, 0, NULL, "access control list" },
  { NULL }
};

int
mutool_acl (int argc, char **argv)
{
  int rc, index;
  mu_acl_result_t result;
  mu_cfg_tree_t *tree = NULL, *temp_tree = NULL;
  mu_cfg_node_t *node;
  
  if (argp_parse (&acl_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

  if (argc == 0)
    {
      mu_error (_("not enough arguments"));
      return 1;
    }

  if (input_file_name)
    {
      mu_load_site_rcfile = 0;
      mu_load_user_rcfile = 0;
      mu_load_rcfile = input_file_name;
    }

  mu_acl_cfg_init ();
  if (mu_libcfg_parse_config (&tree))
    return 1;
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

      target_sa = parse_address (&target_salen, ap);
      mu_printf ("Testing %s:\n", ap);
      rc = mu_acl_check_sockaddr (acl, target_sa, target_salen, &result);
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
