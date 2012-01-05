/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
mu_property_clear (mu_property_t prop)
{
  int rc = _mu_property_check (prop);
  if (rc)
    return rc;
  if (!prop->_prop_clear)
    return MU_ERR_EMPTY_VFN;
  rc = prop->_prop_clear (prop);
  if (rc == 0)
    prop->_prop_flags |= MU_PROP_MODIFIED;
  return rc;
}
