/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2008 Free Software Foundation, Inc.

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

static struct mu_cmdline_capa *all_cmdline_capa[] = {
  &mu_common_cmdline,
  &mu_logging_cmdline,
  &mu_license_cmdline,
  &mu_mailbox_cmdline,
  &mu_locking_cmdline,
  &mu_address_cmdline,
  &mu_mailer_cmdline,
  &mu_debug_cmdline,
  &mu_pam_cmdline,
  &mu_gsasl_cmdline,
  &mu_tls_cmdline,
  &mu_radius_cmdline,
  &mu_sql_cmdline,
  &mu_virtdomain_cmdline,
  &mu_auth_cmdline,
  &mu_sieve_cmdline,
  NULL
};

static int libargp_init_passed = 0;

void
mu_libargp_init ()
{
  struct mu_cmdline_capa **cpp;
  if (libargp_init_passed)
    return;
  libargp_init_passed = 1;
  for (cpp = all_cmdline_capa; *cpp; cpp++)
    {
      struct mu_cmdline_capa *cp = *cpp;
      if (mu_register_argp_capa (cp->name, cp->child))
	{
	  mu_error (_("INTERNAL ERROR: cannot register argp capability `%s'"),
		    cp->name);
	  abort ();
	}
    }
}

void
mu_argp_node_list_init (struct mu_argp_node_list *lst)
{
  lst->count = 0;
  lst->head = lst->tail = NULL;
}

void
mu_argp_node_list_add (struct mu_argp_node_list *lst,  mu_cfg_node_t *node)
{
  lst->count++;
  if (lst->tail)
    lst->tail->next = node;
  else
    lst->head = node;
  lst->tail = node;
}
		   
void
mu_argp_node_list_new (struct mu_argp_node_list *lst,
		       const char *tag, const char *label)
{
  mu_cfg_node_t *node;
  mu_cfg_locus_t loc = { "command line", 0 };

  loc.line = lst->count;
  node = mu_cfg_tree_create_node (mu_argp_tree, mu_cfg_node_param,
				  &loc, tag, label, NULL);
  mu_argp_node_list_add (lst, node);
}

void
mu_argp_node_list_finish (struct mu_argp_node_list *lst, char *tag,
			  char *label)
{
  mu_cfg_node_t *node;

  if (!lst->head)
    return;
  if (tag)
    node = mu_cfg_tree_create_node (mu_argp_tree,
				    mu_cfg_node_tag,
				    NULL,
				    tag, label,
				    lst->head);
  else
    node = lst->head;
  mu_cfg_tree_add_node (mu_argp_tree, node);
  mu_argp_node_list_init (lst);
}
