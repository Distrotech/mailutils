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
mu_property_sget_value (mu_property_t prop, const char *key,
			const char **pval)
{
  int rc = _mu_property_check (prop);
  if (rc)
    return rc;
  if (!prop->_prop_getval)
    return MU_ERR_EMPTY_VFN;
  return prop->_prop_getval (prop, key, pval);
}

int
mu_property_aget_value (mu_property_t prop, const char *key,
			char **buffer)
{
  const char *value;
  int rc = mu_property_sget_value (prop, key, &value);
  if (rc == 0)
    {
      if ((*buffer = strdup (value)) == NULL)
	return ENOMEM;
    }
  return rc;
}

int
mu_property_get_value (mu_property_t prop, const char *key, char *buffer,
		       size_t buflen, size_t *n)
{
  size_t len = 0;
  const char *value;
  int rc = mu_property_sget_value (prop, key, &value);
  if (rc == 0)
    {
      len = strlen (value) + 1;
      if (buffer && buflen)
	{
	  if (buflen < len)
	    len = buflen;
	  len--;
	  memcpy (buffer, value, len);
	  buffer[len] = 0;
	}
    }
  if (n)
    *n = len;
  return rc;
}

int
mu_property_is_set (mu_property_t prop, const char *key)
{
  if (_mu_property_check (prop))
    return 0;
  return mu_property_sget_value (prop, key, NULL) == 0;
}
