/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <property0.h>

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

static int property_find __P ((property_t, const char *,
			       struct property_item **));

int
property_create (property_t *pp, void *owner)
{
  property_t prop;
  if (pp == NULL)
    return EINVAL;
  prop = calloc (1, sizeof *prop);
  if (prop == NULL)
    return ENOMEM;
  monitor_create (&prop->lock, 0, prop);
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
	  struct property_item *item, *cur;
	  /* Destroy the list and is properties.  */
	  monitor_wrlock (prop->lock);
	  for (item = prop->items; item; item = cur)
	    {
	      if (item->key)
		free (item->key);
	      if (item->value)
		free (item->value);
	      cur = item->next;
	      free (item);
	    }
	  monitor_unlock (prop->lock);
	  monitor_destroy (&prop->lock, prop);
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
property_set_value (property_t prop, const char *key,  const char *value,
		    int overwrite)
{
  struct property_item *item;
  int status = property_find (prop, key, &item);
  if (status != 0)
    return status;

  if (item->set)
    {
      if (overwrite)
        {
          item->set = 0;
          if (item->value)
            free (item->value);
          item->value = NULL;
          if (value)
            {
              item->set = 1;
              item->value = strdup (value);
	      if (item->value == NULL)
		status = ENOMEM;
            }
        }
    }
  else
    {
      item->set = 1;
      if (item->value)
        free (item->value);
      if (value)
	{
	  item->value = strdup (value);
	  if (item->value == NULL)
	    status = ENOMEM;
	}
    }
  return status;
}

int
property_get_value (property_t prop, const char *key, char *buffer,
		    size_t buflen, size_t *n)
{
  struct property_item *item = NULL;
  int status;
  size_t len;

  status = property_find (prop, key, &item);
  if (status != 0)
    return status;

  len = (item->value) ? strlen (item->value) : 0;
  if (buffer && buflen != 0)
    {
      buflen--;
      len = min (buflen, len);
      strncpy (buffer, item->value, len)[len] = '\0';
    }
  if (n)
    *n = len;
  return 0;
}

int
property_set (property_t prop, const char *k)
{
  struct property_item *item = NULL;
  int status = property_find (prop, k, &item);
  if (status != 0)
    return status;
  item->set = 1;
  return 0;
}

int
property_unset (property_t prop, const char *k)
{
  struct property_item *item = NULL;
  int status = property_find (prop, k, &item);
  if (status != 0)
    return status;
  item->set = 0;
  return 0;
}

int
property_is_set (property_t prop, const char *k)
{
  struct property_item *item = NULL;
  int status = property_find (prop, k, &item);
  if (status != 0)
    return 0;
  return item->set;
}

static int
property_find (property_t prop, const char *key, struct property_item **item)
{
  size_t len = 0;
  struct property_item *cur = NULL;

  if (prop == NULL || key == NULL)
    return EINVAL;

  monitor_wrlock (prop->lock);
  for (len = strlen (key), cur = prop->items; cur; cur = cur->next)
    {
      if (strlen (cur->key) == len && !strcmp (key, cur->key))
	break;
    }

  if (cur == NULL)
    {
      cur = calloc (1, sizeof *cur);
      if (cur == NULL)
	{
	  monitor_unlock (prop->lock);
	  return ENOMEM;
	}
      cur->key = strdup (key);
      if (cur->key == NULL)
	{
	  monitor_unlock (prop->lock);
	  free (cur);
	  return ENOMEM;
	}
      cur->next = prop->items;
      prop->items = cur;
    }
  *item = cur;
  monitor_unlock (prop->lock);
  return 0;
}
