/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifdef DMALLOC
# include <dmalloc.h>
#endif

struct property_data
{
  size_t hash;
  const void *value;
};

struct _property
{
  void *owner;
  list_t list;
  int (*_set_value) __P ((property_t , const char *, const void *));
  int (*_get_value) __P ((property_t, const char *, void **));
};

static int property_find __P ((list_t, const char *, struct property_data **));
static size_t hash __P ((const char *));

int
property_create (property_t *pp, void *owner)
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
  prop->owner = owner;
  *pp = prop;
  return 0;
}

void
property_destroy (property_t *pp, void *owner)
{
  if (pp && *pp)
    {
      property_t prop = *pp;
      if (prop->owner == owner)
	{
	  list_destroy (&(prop->list));
	  free (prop);
	}
      *pp = NULL;
    }
}

void *
property_get_owner (property_t prop)
{
  return (prop == NULL) ? NULL : prop->owner;
}

int
property_set_set_value (property_t prop, int (*_set_value)
			__P ((property_t , const char *, const void *)),
			void *owner)
{
  if (prop == NULL)
    return EINVAL;
  if (prop->owner != owner)
    return EACCES;
  prop->_set_value = _set_value;
  return 0;
}

int
property_set_value (property_t prop, const char *key,  const void *value)
{
  struct property_data *pd = NULL;
  int status;

  if (prop == NULL)
    return EINVAL;

  if (prop->_set_value)
    return prop->_set_value (prop, key, value);

  status = property_find (prop->list, key, &pd);
  if (status != 0)
    return status;

  if (pd == NULL)
    {
      pd = calloc (1, sizeof (*pd));
      if (pd == NULL)
	return ENOMEM;
      pd->hash = hash (key);
      pd->value = (void *)value;
      list_append (prop->list, (void *)pd);
    }
  else
    pd->value = (void *)value;
  return 0;
}

int
property_set_get_value (property_t prop, int (*_get_value)
			__P ((property_t, const char *, void **)),
			void *owner)
{
  if (prop == NULL)
    return EINVAL;
  if (prop->owner != owner)
    return EACCES;
  prop->_get_value = _get_value;
  return 0;
}

int
property_get_value (property_t prop, const char *key, void **pvalue)
{
  struct property_data *pd = NULL;
  int status;

  if (prop == NULL)
    return EINVAL;

  if (prop->_get_value)
    return prop->_get_value (prop, key, pvalue);

  status = property_find (prop->list, key, &pd);
  if (status != 0)
    return status;

  if (pd == NULL)
    return ENOENT;

  if (pvalue)
    *pvalue =  (void *)pd->value;
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
  return property_set_value (prop, k, (void *)i);
}

int
property_get_int (property_t prop, const char *k)
{
  int value = 0;
  property_get_value (prop, k, (void **)&value);
  return value;
}

int
property_set_long (property_t prop, const char *k, long l)
{
  return property_set_value (prop, k, (const void *)l);
}

long
property_get_long (property_t prop, const char *k)
{
  long value = 0;
  property_get_value (prop, k, (void **)&value);
  return value;
}

int
property_set_pointer (property_t prop, const char *k, void *p)
{
  return property_set_value (prop, k, p);
}

void *
property_get_pointer (property_t prop, const char *k)
{
  void *value = NULL;
  property_get_value (prop, k, &value);
  return value;
}

/* Taking from an article in Dr Dobbs.  */
static size_t
hash (const char *s)
{
  size_t hashval;
  for (hashval = 0; *s != '\0' ; s++)
    {
      hashval  += (unsigned)*s ;
      hashval  += (hashval << 10);
      hashval  ^= (hashval >> 6) ;
    }
  hashval += (hashval << 3);
  hashval ^= (hashval >> 11);
  hashval += (hashval << 15);
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
