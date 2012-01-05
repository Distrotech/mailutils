/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/property.h>
#include <mailutils/iterator.h>
#include <mailutils/errno.h>
#include <mailutils/mh.h>

mu_property_t mu_mh_profile;
mu_property_t mu_mh_context;

const char *
mu_mhprop_get_value (mu_property_t prop, const char *name, const char *defval)
{
  const char *p;

  if (!prop || mu_property_sget_value (prop, name, &p))
    p = defval; 
  return p;
}

int
mu_mhprop_iterate (mu_property_t prop, mu_mhprop_iterator_t fp, void *data)
{
  mu_iterator_t itr;
  int rc;

  if (!prop)
    return EINVAL;
  rc = mu_property_get_iterator (prop, &itr);
  if (rc)
    return rc;
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      const char *name, *val;
      mu_iterator_current_kv (itr, (const void **)&name, (void**)&val);
      rc = fp (name, val, data);
      if (rc)
	break;
    }
  mu_iterator_destroy (&itr);
  return rc;
}
