/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005, 2007-2008, 2010-2012 Free Software
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
   <http://www.gnu.org/licenses/>. */; 

#ifndef _MAILUTILS_PROPERTY_H
#define _MAILUTILS_PROPERTY_H

#include <sys/types.h>

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int mu_property_create   (mu_property_t *);
int mu_property_create_init (mu_property_t *pprop,
			     int (*initfun) (mu_property_t), void *initdata);
int mu_property_set_init (mu_property_t prop,
			  int (*initfun) (mu_property_t), void *initdata);
int mu_property_set_init_data (mu_property_t prop, void *data,
			       void **old_data);
void mu_property_destroy (mu_property_t *pprop);

void mu_property_ref (mu_property_t prop);
void mu_property_unref (mu_property_t prop);
int mu_property_save (mu_property_t prop);
int mu_property_set_value (mu_property_t, const char *, const char *, int);
int mu_property_get_value (mu_property_t, const char *, char *, size_t, 
			   size_t *);
int mu_property_sget_value (mu_property_t prop, const char *key,
			    const char **buffer);
int mu_property_aget_value (mu_property_t prop, const char *key,
			    char **buffer);
int mu_property_clear (mu_property_t prop);
int mu_property_invalidate (mu_property_t prop);

/* Helper functions.  */
int mu_property_set  (mu_property_t, const char *);
int mu_property_unset (mu_property_t, const char *);
int mu_property_is_set (mu_property_t, const char *);

int mu_property_get_iterator (mu_property_t, mu_iterator_t *itr);
  
/* Implementation init functions */
int mu_assoc_property_init (mu_property_t);

struct mu_mh_prop
{
  char *filename;
  int ro;
};

int mu_mh_property_init (mu_property_t);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_PROPERTY_H */
