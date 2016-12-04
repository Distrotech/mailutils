/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2005, 2007-2008, 2010-2012, 2014-2016
   Free Software Foundation, Inc.

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

int
mu_sieve_registry_require (mu_sieve_machine_t mach, const char *name,
			   enum mu_sieve_record type)
{
  mu_sieve_registry_t *reg;

  reg = mu_sieve_registry_lookup (mach, name, type);
  if (!reg)
    {
      void *handle = mu_sieve_load_ext (mach, name);
      if (!handle)
	return 1;
      reg = mu_sieve_registry_lookup (mach, name, type);
      if (!reg)
	return 1;
      reg->handle = handle;
    }

  reg->required = 1;
  return 0;
}


static void
regunload (void *data)
{
  mu_sieve_registry_t *reg = data;
  mu_sieve_unload_ext (reg->handle);
}

static int
regcmp (void const *a, void const *b)
{
  mu_sieve_registry_t const *rega = a;
  mu_sieve_registry_t const *regb = b;
  if (rega->type != regb->type)
    return rega->type - regb->type;
  return strcmp (rega->name, regb->name);
}

mu_sieve_registry_t *
mu_sieve_registry_add (mu_sieve_machine_t mach, const char *name)
{
  mu_sieve_registry_t *reg;
  int rc;
  
  if (!mach->registry)
    {
      rc = mu_list_create (&mach->registry);
      if (rc)
	{
	  mu_sieve_error (mach, "mu_list_create: %s", mu_strerror (rc));
	  mu_sieve_abort (mach);
	}
      mu_list_set_destroy_item (mach->registry, regunload);
      mu_list_set_comparator (mach->registry, regcmp);
    }
  reg = mu_sieve_malloc (mach, sizeof (*reg));
  reg->name = name;
  reg->handle = NULL;
  reg->required = 0;
  memset (&reg->v, 0, sizeof reg->v);
  rc = mu_list_append (mach->registry, reg);
  if (rc)
    {
      mu_sieve_error (mach, "mu_list_append: %s", mu_strerror (rc));
      mu_sieve_abort (mach);
    }
  return reg;
}

mu_sieve_registry_t *
mu_sieve_registry_lookup (mu_sieve_machine_t mach, const char *name,
			  enum mu_sieve_record type)
{
  mu_sieve_registry_t key, *reg;
  int rc;
  
  key.name = name;
  key.type = type;

  rc = mu_list_locate (mach->registry, &key, (void**) &reg);
  if (rc == MU_ERR_NOENT)
    return NULL;
  else if (rc)
    {
      mu_sieve_error (mach, _("registry lookup failed: %s"), mu_strerror (rc));
      mu_sieve_abort (mach);
    }
  return reg;
}

void
mu_sieve_register_test_ext (mu_sieve_machine_t mach,
			    const char *name, mu_sieve_handler_t handler,
			    mu_sieve_data_type *req_args,
			    mu_sieve_data_type *opt_args,
			    mu_sieve_tag_group_t *tags, int required)
{
  mu_sieve_registry_t *reg = mu_sieve_registry_add (mach, name);

  reg->type = mu_sieve_record_test;
  reg->required = required;
  reg->v.command.handler = handler;
  reg->v.command.req_args = req_args;
  reg->v.command.opt_args = opt_args;
  reg->v.command.tags = tags;
}

void
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

void
mu_sieve_register_action_ext (mu_sieve_machine_t mach,
			      const char *name, mu_sieve_handler_t handler,
			      mu_sieve_data_type *req_args,
			      mu_sieve_data_type *opt_args,
			      mu_sieve_tag_group_t *tags, int required)
{
  mu_sieve_registry_t *reg = mu_sieve_registry_add (mach, name);

  reg->type = mu_sieve_record_action;
  reg->required = required;
  reg->v.command.handler = handler;
  reg->v.command.req_args = req_args;
  reg->v.command.opt_args = opt_args;
  reg->v.command.tags = tags;
}

void
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
