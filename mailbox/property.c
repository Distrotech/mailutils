/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005  Free Software Foundation, Inc.

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
#include <config.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/errno.h>
#include <property0.h>

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

static int property_find (mu_property_t, const char *, struct property_item **);

int
mu_property_create (mu_property_t *pp, void *owner)
{
  mu_property_t prop;
  if (pp == NULL)
    return MU_ERR_OUT_PTR_NULL;
  prop = calloc (1, sizeof *prop);
  if (prop == NULL)
    return ENOMEM;
  mu_monitor_create (&prop->lock, 0, prop);
  prop->owner = owner;
  *pp = prop;
  return 0;
}

void
mu_property_destroy (mu_property_t *pp, void *owner)
{
  if (pp && *pp)
    {
      mu_property_t prop = *pp;
      if (prop->owner == owner)
	{
	  struct property_item *item, *cur;
	  /* Destroy the list and is properties.  */
	  mu_monitor_wrlock (prop->lock);
	  for (item = prop->items; item; item = cur)
	    {
	      if (item->key)
		free (item->key);
	      if (item->value)
		free (item->value);
	      cur = item->next;
	      free (item);
	    }
	  mu_monitor_unlock (prop->lock);
	  mu_monitor_destroy (&prop->lock, prop);
	  free (prop);
	}
      *pp = NULL;
    }
}

void *
mu_property_get_owner (mu_property_t prop)
{
  return (prop == NULL) ? NULL : prop->owner;
}

int
mu_property_set_value (mu_property_t prop, const char *key,  const char *value,
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
mu_property_get_value (mu_property_t prop, const char *key, char *buffer,
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
mu_property_sget_value (mu_property_t prop, const char *key,
			const char **buffer)
{
  struct property_item *item = NULL;
  int status;

  status = property_find (prop, key, &item);
  if (status == 0)
    *buffer = item->value;
  return status;
}

int
mu_property_aget_value (mu_property_t prop, const char *key,
			char **buffer)
{
  struct property_item *item = NULL;
  int status;

  status = property_find (prop, key, &item);
  if (status == 0)
    {
      *buffer = strdup (item->value);
      if (!*buffer)
	status = ENOMEM;
    }
  return status;
}

int
mu_property_set (mu_property_t prop, const char *k)
{
  struct property_item *item = NULL;
  int status = property_find (prop, k, &item);
  if (status != 0)
    return status;
  item->set = 1;
  return 0;
}

int
mu_property_unset (mu_property_t prop, const char *k)
{
  struct property_item *item = NULL;
  int status = property_find (prop, k, &item);
  if (status != 0)
    return status;
  item->set = 0;
  return 0;
}

int
mu_property_is_set (mu_property_t prop, const char *k)
{
  struct property_item *item = NULL;
  int status = property_find (prop, k, &item);
  if (status != 0)
    return 0;
  return item->set;
}

static int
property_find (mu_property_t prop, const char *key, struct property_item **item)
{
  size_t len = 0;
  struct property_item *cur = NULL;

  if (prop == NULL || key == NULL)
    return EINVAL;

  mu_monitor_wrlock (prop->lock);
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
	  mu_monitor_unlock (prop->lock);
	  return ENOMEM;
	}
      cur->key = strdup (key);
      if (cur->key == NULL)
	{
	  mu_monitor_unlock (prop->lock);
	  free (cur);
	  return ENOMEM;
	}
      cur->next = prop->items;
      prop->items = cur;
    }
  *item = cur;
  mu_monitor_unlock (prop->lock);
  return 0;
}
