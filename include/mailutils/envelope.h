/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2005-2007, 2010-2012 Free Software
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

#ifndef _MAILUTILS_ENVELOPE_H
# define _MAILUTILS_ENVELOPE_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" { /*}*/
#endif

int mu_envelope_create (mu_envelope_t *, void *);
void mu_envelope_destroy (mu_envelope_t *, void *);
void *mu_envelope_get_owner (mu_envelope_t);
int mu_envelope_get_sender (mu_envelope_t, char *, size_t, size_t *);
int mu_envelope_sget_sender (mu_envelope_t, const char **);
int mu_envelope_aget_sender (mu_envelope_t, char **);
int mu_envelope_get_date (mu_envelope_t, char *, size_t, size_t *);
int mu_envelope_sget_date (mu_envelope_t, const char **);
int mu_envelope_aget_date (mu_envelope_t, char **);

#define mu_envelope_sender mu_envelope_get_sender
#define mu_envelope_date   mu_envelope_get_date
  
int mu_envelope_set_sender (mu_envelope_t, 
			    int (*_sender) (mu_envelope_t,
					    char *, size_t, size_t*), 

			    void *);
int mu_envelope_set_date (mu_envelope_t, 
			  int (*_date) (mu_envelope_t, char *, size_t ,
					size_t *), 
			  void *);
int mu_envelope_set_destroy (mu_envelope_t envelope,
			     int (*_destroy) (mu_envelope_t),
			     void *owner);

#ifdef __cplusplus
}
#endif
#endif /* _MAILUTILS_ENVELOPE_H */
