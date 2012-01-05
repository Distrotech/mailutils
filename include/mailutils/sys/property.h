/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007-2008, 2010-2012 Free Software
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

#define MU_PROP_INIT     0x01
#define MU_PROP_FILL     0x02
#define MU_PROP_MODIFIED 0x04
  
struct _mu_property
{
  size_t _prop_ref_count;    /* Reference counter */
  int _prop_flags;           /* Flags describing the state of this object */
  void *_prop_data;          /* Implementation-specific data */
  void *_prop_init_data;     /* Initialization data */

  /* Methods */

  /* Delayed initialization.  This function must allocate _prop_data,
     if needed, and initialize the rest of _prop_* methods. */
  int (*_prop_init)  (struct _mu_property *);
  /* Free memory allocated in _prop_data */
  void (*_prop_done) (struct _mu_property *);
  /* Fill in the properties from an external storage */
  int (*_prop_fill)  (struct _mu_property *);
  /* Write the properties to an external storage */
  int (*_prop_save)  (struct _mu_property *);
  /* Get the value of the property named in the 2nd argument.  If 3rd
     arg is NULL, _prop_getval tests whether the given property is set. */
  int (*_prop_getval) (struct _mu_property *, const char *, const char **);
  /* Set the property */
  int (*_prop_setval) (struct _mu_property *, const char *, const char *, int);
  /* Unset (delete) the property */
  int (*_prop_unset) (struct _mu_property *, const char *);
  /* Return iterator for this property object */
  int (*_prop_getitr) (struct _mu_property *, mu_iterator_t *);
  /* Clear all properties. */
  int (*_prop_clear) (struct _mu_property *);
};

int _mu_property_init (mu_property_t prop);
int _mu_property_check (mu_property_t prop);
  
# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_PROPERTY_H */
