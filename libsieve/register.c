/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve.h>

static list_t test_list;
static list_t action_list;

static sieve_register_t *
sieve_lookup (list_t list, const char *name)
{
  iterator_t itr;
  sieve_register_t *reg;

  if (!list || iterator_create (&itr, list))
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
sieve_test_lookup (const char *name)
{
  return sieve_lookup (test_list, name);
}

sieve_register_t *
sieve_action_lookup (const char *name)
{
  return sieve_lookup (action_list, name);
}
     
static int
sieve_register (list_t *list,
		const char *name, sieve_handler_t handler,
		sieve_data_type *arg_types,
		sieve_tag_def_t *tags, int required)
{
  int n;
  sieve_register_t *reg = malloc (sizeof (*reg));

  if (!reg)
    return ENOMEM;
  reg->name = name;
  reg->handler = handler;

  if (arg_types)
    {
      for (n = 0; arg_types[n] != SVT_VOID; n++)
	;
    }
  else
    n = 0;
  reg->num_req_args = n;
  reg->req_args = arg_types;

  if (tags)
    {
      for (n = 0; tags[n].name != NULL; n++)
	;
    }
  else
    n = 0;
  reg->num_tags = n;
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
sieve_register_test (const char *name, sieve_handler_t handler,
		     sieve_data_type *arg_types,
		     sieve_tag_def_t *tags, int required)
{
  return sieve_register (&test_list, name, handler, arg_types, tags, required);
}

int
sieve_register_action (const char *name, sieve_handler_t handler,
		       sieve_data_type *arg_types,
		       sieve_tag_def_t *tags, int required)
{
  return sieve_register (&action_list, name, handler, arg_types, tags, required);
}
