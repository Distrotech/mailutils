/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>

#include <mailutils/property.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>

struct property_data
{
  size_t hash;
  enum property_type type;
  union
  {
    long l;
    void *v;
  } value;
};

struct _property
{
  list_t list;
};

static int property_find __P ((list_t, const char *, struct property_data **));
static size_t hash __P ((const char *));

int
property_create (property_t *pp)
{
  property_t prop;
  int status;
  if (pp == NULL)
    return EINVAL;
  prop = calloc (1, sizeof (*prop));
  if (prop == NULL)
    return ENOMEM;
  status = list_create (&(prop->list));
  if (status != 0)
    {
      free (prop);
      return status;
    }
  *pp = prop;
  return 0;
}

void
property_destroy (property_t *pp)
{
  if (pp && *pp)
    {
      property_t prop = *pp;
      list_destroy (&(prop->list));
      free (prop);
      *pp = NULL;
    }
}


int
property_set_value (property_t prop, const char *key,  const void *value,
		    enum property_type type)
{
  struct property_data *pd;
  int status;

  if (prop == NULL)
    return EINVAL;

  status = property_find (prop->list, key, &pd);
  if (status != 0)
    return status;

  if (pd == NULL)
    {
      pd = calloc (1, sizeof (*pd));
      if (pd == NULL)
	return ENOMEM;
      pd->hash = hash (key);
      pd->type = type;
      switch (type)
	{
	case TYPE_POINTER:
	  pd->value.v = (void *)value;
	  break;

	case TYPE_LONG:
	  pd->value.l = (long)value;
	  break;
	}
      list_append (prop->list, (void *)pd);
    }
  else
    pd->value.v = (void *)value;
  return 0;
}

int
property_get_value (property_t prop, const char *key, void **pvalue,
		    enum property_type *ptype)
{
  struct property_data *pd;
  int status;

  if (prop == NULL)
    return EINVAL;

  status = property_find (prop->list, key, &pd);
  if (status != 0)
    return status;

  if (pd == NULL)
    return ENOENT;

  if (ptype)
    *ptype = pd->type;
  if (pvalue)
    *pvalue =  pd->value.v;
  return 0;
}

int
property_set  (property_t prop, const char *k)
{
  return property_set_int (prop, k, 1);
}

int
property_unset (property_t prop, const char *k)
{
  return property_set_int (prop, k, 0);
}

int
property_is_set (property_t prop, const char *k)
{
  return property_get_int (prop, k);
}

int
property_set_int (property_t prop, const char *k, int i)
{
  return property_set_value (prop, k, (void *)i, TYPE_LONG);
}

int
property_get_int (property_t prop, const char *k)
{
  int value = 0;
  property_get_value (prop, k, (void **)&value, NULL);
  return value;
}

int
property_set_long (property_t prop, const char *k, long l)
{
  return property_set_value (prop, k, (const void *)l, TYPE_LONG);
}

long
property_get_long (property_t prop, const char *k)
{
  long value = 0;
  property_get_value (prop, k, (void **)&value, NULL);
  return value;
}

int
property_set_pointer (property_t prop, const char *k, void *p)
{
  return property_set_value (prop, k, p, TYPE_POINTER);
}

void *
property_get_pointer (property_t prop, const char *k)
{
  void *value = NULL;
  property_get_value (prop, k, &value, NULL);
  return value;
}

static size_t
hash (const char *s)
{
  size_t hashval;
  for (hashval = 0; *s != '\0' ; s++)
    {
      hashval  += (unsigned)*s ;
      hashval  += (hashval <<10);
      hashval  ^= (hashval >>6) ;
    }
  hashval  += (hashval  <<3);
  hashval  ^= (hashval  >>11);
  hashval  += (hashval  <<15);
  return hashval;
}

static int
property_find (list_t list, const char *key, struct property_data **p)
{
  int status;
  size_t h;
  struct property_data *pd = NULL;
  iterator_t iterator;

  status = iterator_create (&iterator, list);
  if (status != 0)
    return status;

  h = hash (key);
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&pd);
      if (pd && pd->hash == h)
        {
	  break;
	}
      pd = NULL;
    }
  iterator_destroy (&iterator);
  *p = pd;
  return 0;
}
