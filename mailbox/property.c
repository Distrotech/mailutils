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

static int property_find __P ((list_t, const char *, struct property_data **));
static int property_add __P ((property_t, const char *, const char *,
			      int (*_set_value)
			      __P ((property_t, const char *, const char *)),
			      int (*_get_value)
			      __P ((property_t, const char *, char *,
				    size_t, size_t *))));
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
	      struct property_data *pd = NULL;
	      iterator_t iterator = NULL;

	      iterator_create (&iterator, prop->list);
	      for (iterator_first (iterator); !iterator_is_done (iterator);
		   iterator_next (iterator))
		{
		  iterator_current (iterator, (void **)&pd);
		  if (pd)
		    {
		      if (pd->key)
			free (pd->key);
		      if (pd->value)
			free (pd->value);
		      free (pd);
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
property_add_defaults (property_t prop, const char *key, const char *value,
		      int (*_set_value) __P ((property_t, const char *,
					      const char *)),
		      int (*_get_value) __P ((property_t, const char *,
					      char *, size_t, size_t *)),
		      void *owner)
{
  if (prop == NULL)
    return EINVAL;
  if (prop->owner != owner)
    return EACCES;
  return property_add (prop, key, value, _set_value, _get_value);
}

int
property_set_value (property_t prop, const char *key,  const char *value)
{

  if (prop == NULL)
    return EINVAL;
  return property_add (prop, key, value, NULL, NULL);
}

int
property_get_value (property_t prop, const char *key, char *buffer,
		    size_t buflen, size_t *n)
{
  struct property_data *pd = NULL;
  int status;
  size_t len;

  if (prop == NULL)
    return EINVAL;

  status = property_find (prop->list, key, &pd);
  if (status != 0)
    return status;

  if (pd == NULL)
    return ENOENT;

  if (pd->_get_value)
    return pd->_get_value (prop, key, buffer, buflen, n);

  len = (pd->value) ? strlen (pd->value) : 0;
  if (buffer && buflen != 0)
    {
      buflen--;
      len = (buflen < len) ? buflen : len;
      strncpy (buffer, pd->value, len)[len] = '\0';
    }
  if (n)
    *n = len;
  return 0;
}

#if 0
int
property_load (property_t prop, stream_t stream)
{
  size_t n = 0;
  off_t off = 0;
  int status;
  int buflen = 512;
  char *buf = calloc (buflen, sizeof (*buf));
  if (buf == NULL)
    return ENOMEM;

  while ((status = stream_readline (stream, buf, buflen, off, &n)) == 0
	 && n > 0)
    {
      char *sep;
      if (buf[n] != '\n')
	{
	  char *tmp;
	  buflen *= 2;
	  tmp = realloc (buf, buflen);
	  if (tmp == NULL)
	    {
	      free (buf);
	      return ENOMEM;
	    }
	  buf = tmp;
	  continue;
	}
      sep = strchr (buf, '=');
      if (sep)
	{
	  *sep++ = '\0';
	  property_set_value (prop, buf, sep);
	}
      else
	property_set (prop, buf);
    }
  return 0;
}
#endif

int
property_set (property_t prop, const char *k)
{
  if (!property_is_set (prop, k))
    return property_set_value (prop, k, "1");
  return 0;
}

int
property_unset (property_t prop, const char *k)
{
  if (property_is_set (prop, k))
    return property_set_value (prop, k, NULL);
  return 0;
}

int
property_is_set (property_t prop, const char *k)
{
  size_t n = 0;
  property_get_value (prop, k, NULL, 0, &n);
  return (n != 0);
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

  h = property_hash (key);
  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&pd);
      if (pd)
        {
	  if (pd->hash == h)
	    if (pd->key && strcasecmp (pd->key, key) == 0)
	      break;
	}
      pd = NULL;
    }
  iterator_destroy (&iterator);
  *p = pd;
  return 0;
}

static int
property_add (property_t prop, const char *key, const char *value,
	      int (*_set_value) __P ((property_t, const char *, const char *)),
	      int (*_get_value) __P ((property_t,  const char *, char *,
				      size_t, size_t *)))
{
  struct property_data *pd = NULL;
  int status;

  if (key == NULL || *key == '\0')
    return EINVAL;

  if (prop->list == NULL)
    {
      status = list_create (&(prop->list));
      if (status != 0)
	return status;
    }

  status = property_find (prop->list, key, &pd);
  if (status != 0)
    return status;

  /* None find create a new one.  */
  if (pd == NULL)
    {
      pd = calloc (1, sizeof (*pd));
      if (pd == NULL)
	return ENOMEM;
      pd->hash = property_hash (key);
      list_append (prop->list, (void *)pd);
    }

  if (pd->key == NULL)
    pd->key = strdup (key);
  if (pd->value)
    free (pd->value);
  pd->value = (value) ? strdup (value) : NULL;
  pd->_set_value = _set_value;
  pd->_get_value = _get_value;
  return 0;
}
