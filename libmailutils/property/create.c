/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007-2008, 2010-2012 Free
   Software Foundation, Inc.

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
#include <config.h>
#endif

#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/sys/property.h>

int
mu_property_create (mu_property_t *pprop)
{
  mu_property_t prop = calloc (1, sizeof (prop[0]));
  if (!prop)
    return ENOMEM;
  *pprop = prop;
  return 0;
}

int
mu_property_create_init (mu_property_t *pprop,
			 int (*initfun) (mu_property_t), void *initdata)
{
  mu_property_t prop;
  int rc = mu_property_create (&prop);
  if (rc == 0)
    {
      mu_property_set_init (prop, initfun, initdata);
      *pprop = prop;
    }
  return rc;
}

int
mu_property_set_init (mu_property_t prop,
		      int (*initfun) (mu_property_t), void *initdata)
{
  if (!prop)
    return ENOMEM;
  if (prop->_prop_flags & MU_PROP_INIT)
    return MU_ERR_SEQ;
  prop->_prop_init = initfun;
  prop->_prop_init_data = initdata;
  return 0;
}

int
mu_property_set_init_data (mu_property_t prop, void *data, void **old_data)
{
  if (!prop)
    return ENOMEM;
  if (prop->_prop_flags & MU_PROP_INIT)
    return MU_ERR_SEQ;
  if (old_data)
    *old_data = prop->_prop_init_data;
  prop->_prop_init_data = data;
  return 0;
}

void
mu_property_destroy (mu_property_t *pprop)
{
  if (pprop)
    {
      mu_property_t prop = *pprop;
      if (prop && (prop->_prop_ref_count == 0 || --prop->_prop_ref_count == 0))
	{
	  mu_property_save (prop);
	  if (prop->_prop_done)
	    prop->_prop_done (prop);
	  free (prop);
	  *pprop = NULL;
	}
    }
}

void
mu_property_ref (mu_property_t prop)
{
  if (prop)
    prop->_prop_ref_count++;
}

void
mu_property_unref (mu_property_t prop)
{
  mu_property_destroy (&prop);
}

int
mu_property_save (mu_property_t prop)
{
  int rc = 0;
      
  if (!prop)
    return EINVAL;
  if (prop->_prop_flags & MU_PROP_MODIFIED)
    {
      if (prop->_prop_save)
	rc = prop->_prop_save (prop);

      if (rc == 0)
	prop->_prop_flags &= ~MU_PROP_MODIFIED;
    }
  return rc;
}

int
_mu_property_init (mu_property_t prop)
{
  int rc = 0;
  if (!(prop->_prop_flags & MU_PROP_INIT))
    {
      if (prop->_prop_init)
	rc = prop->_prop_init (prop);
      if (rc == 0)
	prop->_prop_flags |= MU_PROP_INIT;
    }
  return rc;
}

static int
_mu_property_fill (mu_property_t prop)
{
  int rc = 0;
  if (!(prop->_prop_flags & MU_PROP_FILL))
    {
      if (prop->_prop_fill)
	rc = prop->_prop_fill (prop);
      if (rc == 0)
	prop->_prop_flags |= MU_PROP_FILL;
    }
  return rc;
}

int
_mu_property_check (mu_property_t prop)
{
  int rc;
  
  if (!prop)
    return EINVAL;
  rc = _mu_property_init (prop);
  if (rc == 0)
    rc = _mu_property_fill (prop);
  return rc;
}

  
