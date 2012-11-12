/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2005, 2007-2008, 2010-2012 Free
   Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve-priv.h>

static mu_sieve_register_t *
reg_lookup (mu_list_t list, const char *name)
{
  mu_iterator_t itr;
  mu_sieve_register_t *reg;

  if (!list || mu_list_get_iterator (list, &itr))
    return NULL;

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      mu_iterator_current (itr, (void **)&reg);
      if (strcmp (reg->name, name) == 0)
	break;
      else
	reg = NULL;
    }
  mu_iterator_destroy (&itr);
  return reg;
}

mu_sieve_register_t *
mu_sieve_test_lookup (mu_sieve_machine_t mach, const char *name)
{
  mu_sieve_register_t *reg = reg_lookup (mach->test_list, name);
  return (reg && reg->handler) ? reg : NULL;
}

mu_sieve_register_t *
mu_sieve_action_lookup (mu_sieve_machine_t mach, const char *name)
{
  mu_sieve_register_t *reg = reg_lookup (mach->action_list, name);
  return (reg && reg->handler) ? reg : NULL;
}

static int
reg_require (mu_sieve_machine_t mach, mu_list_t list, const char *name)
{
  mu_sieve_register_t *reg = reg_lookup (list, name);
  if (!reg)
    {
      if (!(mu_sieve_load_ext (mach, name) == 0
	    && (reg = reg_lookup (list, name)) != NULL))
	return 1;
    }
  reg->required = 1;
  return 0;
}

int
mu_sieve_require_action (mu_sieve_machine_t mach, const char *name)
{
  return reg_require (mach, mach->action_list, name);
}

int
mu_sieve_require_test (mu_sieve_machine_t mach, const char *name)
{
  return reg_require (mach, mach->test_list, name);
}


static int
sieve_register (mu_list_t *pool,
		mu_list_t *list,
		const char *name, mu_sieve_handler_t handler,
		mu_sieve_data_type *req_arg_types,
		mu_sieve_data_type *opt_arg_types,
		mu_sieve_tag_group_t *tags, int required)
{
  mu_sieve_register_t *reg = mu_sieve_palloc (pool, sizeof (*reg));

  if (!reg)
    return ENOMEM;
  reg->name = name;
  reg->handler = handler;

  reg->req_args = req_arg_types;
  reg->opt_args = opt_arg_types;
  reg->tags = tags;
  reg->required = required;
  
  if (!*list)
    {
      int rc = mu_list_create (list);
      if (rc)
	{
	  free (reg);
	  return rc;
	}
    }
  
  return mu_list_append (*list, reg);
}


int
mu_sieve_register_test_ext (mu_sieve_machine_t mach,
			    const char *name, mu_sieve_handler_t handler,
			    mu_sieve_data_type *req_args,
			    mu_sieve_data_type *opt_args,
			    mu_sieve_tag_group_t *tags, int required)
{
  return sieve_register (&mach->memory_pool,
			 &mach->test_list, name, handler,
			 req_args, opt_args, tags, required);
}

int
mu_sieve_register_test (mu_sieve_machine_t mach,
			const char *name, mu_sieve_handler_t handler,
			mu_sieve_data_type *arg_types,
			mu_sieve_tag_group_t *tags, int required)
{
  return mu_sieve_register_test_ext (mach, name, handler,
				     arg_types, NULL,
				     tags,
				     required);
}

int
mu_sieve_register_action_ext (mu_sieve_machine_t mach,
			      const char *name, mu_sieve_handler_t handler,
			      mu_sieve_data_type *req_args,
			      mu_sieve_data_type *opt_args,
			      mu_sieve_tag_group_t *tags, int required)
{
  return sieve_register (&mach->memory_pool,
			 &mach->action_list, name, handler,
			 req_args, opt_args, tags, required);
}

int
mu_sieve_register_action (mu_sieve_machine_t mach,
			  const char *name, mu_sieve_handler_t handler,
			  mu_sieve_data_type *arg_types,
			  mu_sieve_tag_group_t *tags, int required)
{
  return mu_sieve_register_action_ext (mach, name, handler,
				       arg_types, NULL,
				       tags,
				       required);
}
