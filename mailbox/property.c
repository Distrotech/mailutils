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

#include <property0.h>

static int property_find __P ((list_t, const char *, struct property_list **));
static int property_add __P ((property_t, const char *, int, int *));
static void property_update __P ((struct property_list *));
static size_t property_hash __P ((const char *));

int
property_create (property_t *pp, void *owner)
{
  property_t prop;
  if (pp == NULL)
    return EINVAL;
  prop = calloc (1, sizeof (*prop));
  if (prop == NULL)
    return ENOMEM;
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
	  /* Destroy the list and is properties.  */
	  if (prop->list)
	    {
	      struct property_list *pl = NULL;
	      iterator_t iterator = NULL;

	      iterator_create (&iterator, prop->list);

	      for (iterator_first (iterator); !iterator_is_done (iterator);
		   iterator_next (iterator))
		{
		  iterator_current (iterator, (void **)&pl);
		  if (pl)
		    {
		      if (pl->key)
			free (pl->key);
		      if (pl->private_)
			free (pl->private_);
		      free (pl);
		    }
		}
	      iterator_destroy (&iterator);
	      list_destroy (&(prop->list));
	    }
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
property_add_default (property_t prop, const char *key, int *address,
		      void *owner)
{
  if (prop == NULL)
    return EINVAL;
  if (prop->owner != owner)
    return EACCES;
  return property_add (prop, key, 0, address);
}

int
property_set_value (property_t prop, const char *key,  int value)
{

  if (prop == NULL)
    return EINVAL;
  return property_add (prop, key, value, NULL);
}

int
property_get_value (property_t prop, const char *key, int *pvalue)
{
  struct property_list *pl = NULL;
  int status;

  if (prop == NULL)
    return EINVAL;

  status = property_find (prop->list, key, &pl);
  if (status != 0)
    return status;

  if (pl == NULL)
    return ENOENT;

  /* Update the value.  */
  property_update (pl);
  if (pvalue)
    *pvalue = pl->value;
  return 0;
}

int
property_set  (property_t prop, const char *k)
{
  return property_set_value (prop, k, 1);
}

int
property_unset (property_t prop, const char *k)
{
  int v = 0;
  property_get_value (prop, k, &v);
  if (v != 0)
    return property_set_value (prop, k, 0);
  return 0;
}

int
property_is_set (property_t prop, const char *k)
{
  int v = 0;
  property_get_value (prop, k, &v);
  return (v != 0);
}

int
property_get_list (property_t prop, list_t *plist)
{
  struct property_list *pl = NULL;
  iterator_t iterator = NULL;
  int status;

  if (plist == NULL || prop == NULL)
    return EINVAL;

  status = iterator_create (&iterator, prop->list);
  if (status != 0)
    return status;

  /* Make sure the values are updated before passing it outside.  */
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&pl);
      if (pl)
	property_update (pl);
    }
  iterator_destroy (&iterator);

  *plist = prop->list;
  return 0;
}

/* Taking from an article in Dr Dobbs.  */
static size_t
property_hash (const char *s)
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

static void
property_update (struct property_list *pl)
{
  /* Update the value.  */
  if (pl->private_)
    {
      struct property_private *private_ = pl->private_;
      if (private_->address)
	pl->value =  *(private_->address);
    }
}

static int
property_find (list_t list, const char *key, struct property_list **p)
{
  int status;
  size_t h;
  struct property_list *pl = NULL;
  iterator_t iterator;

  status = iterator_create (&iterator, list);
  if (status != 0)
    return status;

  h = property_hash (key);
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&pl);
      if (pl)
        {
	  struct property_private *private_ = pl->private_;
	  if (private_->hash == h)
	    if (strcasecmp (pl->key, key) == 0)
	      break;
	}
      pl = NULL;
    }
  iterator_destroy (&iterator);
  *p = pl;
  return 0;
}

static int
property_add (property_t prop, const char *key, int value, int *address)
{
  struct property_list *pl = NULL;
  int status;

  if (key == NULL)
    return EINVAL;

  if (prop->list == NULL)
    {
      status = list_create (&(prop->list));
      if (status != 0)
	return status;
    }

  status = property_find (prop->list, key, &pl);
  if (status != 0)
    return status;

  /* None find create a new one.  */
  if (pl == NULL)
    {
      struct property_private *private_;
      pl = calloc (1, sizeof (*pl));
      if (pl == NULL)
	return ENOMEM;
      private_ = calloc (1, sizeof (*private_));
      if (private_ == NULL)
	{
	  free (pl);
	  return ENOMEM;
	}
      pl->key = strdup (key);
      if (pl->key == NULL)
	{
	  free (private_);
	  free (pl);
	  return ENOMEM;
	}
      pl->value = value;
      private_->hash = property_hash (key);
      private_->address = address;
      pl->private_ = private_;
      list_append (prop->list, (void *)pl);
    }
  else
    {
      struct property_private *private_ = pl->private_;
      if (address)
	private_->address = address;
      else
	{
	  if (private_->address)
	    *(private_->address) = value;
	  pl->value = value;
	}
    }
  return 0;
}
