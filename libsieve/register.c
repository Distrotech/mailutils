/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve.h>

static sieve_register_t *
reg_lookup (list_t list, const char *name)
{
  iterator_t itr;
  sieve_register_t *reg;

  if (!list || list_get_iterator (list, &itr))
    return NULL;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&reg);
      if (strcmp (reg->name, name) == 0)
	break;
      else
	reg = NULL;
    }
  iterator_destroy (&itr);
  return reg;
}

sieve_register_t *
sieve_test_lookup (sieve_machine_t mach, const char *name)
{
  return reg_lookup (mach->test_list, name);
}

sieve_register_t *
sieve_action_lookup (sieve_machine_t mach, const char *name)
{
  return reg_lookup (mach->action_list, name);
}

static int
reg_require (sieve_machine_t mach, list_t list, const char *name)
{
  sieve_register_t *reg = reg_lookup (list, name);
  if (!reg)
    {
      if (!(sieve_load_ext (mach, name) == 0
	    && (reg = reg_lookup (list, name)) != NULL))
	return 1;
    }
  reg->required = 1;
  return 0;
}

int
sieve_require_action (sieve_machine_t mach, const char *name)
{
  return reg_require (mach, mach->action_list, name);
}

int
sieve_require_test (sieve_machine_t mach, const char *name)
{
  return reg_require (mach, mach->test_list, name);
}


static int
sieve_register (list_t *pool,
		list_t *list,
		const char *name, sieve_handler_t handler,
		sieve_data_type *arg_types,
		sieve_tag_group_t *tags, int required)
{
  sieve_register_t *reg = sieve_palloc (pool, sizeof (*reg));

  if (!reg)
    return ENOMEM;
  reg->name = name;
  reg->handler = handler;

  reg->req_args = arg_types;
  reg->tags = tags;
  reg->required = required;
  
  if (!*list)
    {
      int rc = list_create (list);
      if (rc)
	{
	  free (reg);
	  return rc;
	}
    }
  
  return list_append (*list, reg);
}


int
sieve_register_test (sieve_machine_t mach,
		     const char *name, sieve_handler_t handler,
		     sieve_data_type *arg_types,
		     sieve_tag_group_t *tags, int required)
{
  return sieve_register (&mach->memory_pool,
			 &mach->test_list, name, handler,
			 arg_types, tags, required);
}

int
sieve_register_action (sieve_machine_t mach,
		       const char *name, sieve_handler_t handler,
		       sieve_data_type *arg_types,
		       sieve_tag_group_t *tags, int required)
{
  return sieve_register (&mach->memory_pool,
			 &mach->action_list, name, handler,
			 arg_types, tags, required);
}
