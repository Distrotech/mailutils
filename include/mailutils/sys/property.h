/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2007, 2008, 2010 Free Software
   Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_PROPERTY_H
# define _MAILUTILS_SYS_PROPERTY_H

# include <errno.h>
# include <stdlib.h>
# include <string.h>

# include <mailutils/property.h>
# include <mailutils/monitor.h>
# include <mailutils/assoc.h>

# ifdef __cplusplus
extern "C" {
# endif

struct property_item
{
  char *value;
};

struct _mu_property
{
  mu_assoc_t assoc;
  void *owner;
  mu_monitor_t lock;
};

# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_PROPERTY_H */
