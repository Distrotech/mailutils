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
#include <sieve.h>

void *
sieve_alloc (size_t size)
{
  char *p = malloc (size);
  if (!p)
    {
      mu_error ("not enough memory");
      abort ();
    }
  return p;
}

void
sieve_slist_destroy (list_t *plist)
{
  iterator_t itr;

  if (!plist || iterator_create (&itr, *plist))
    return;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *s;
      iterator_current (itr, (void **)&s);
      free (s);
    }
  iterator_destroy (&itr);
  list_destroy (plist);
}

sieve_value_t *
sieve_value_create (sieve_data_type type, void *data)
{
  sieve_value_t *val = sieve_alloc (sizeof (*val));

  val->type = type;
  switch (type)
    {
    case SVT_NUMBER:
      val->v.number = * (long *) data;
      break;
      
    case SVT_STRING:
      val->v.string = data;
      break;
      
    case SVT_VALUE_LIST:
    case SVT_STRING_LIST:
      val->v.list = data;
      
    case SVT_TAG:
    case SVT_IDENT:
      val->v.string = data;
      break;

    default:
      sieve_error ("Invalid data type");
      abort ();
    }
  return val;
}
    
