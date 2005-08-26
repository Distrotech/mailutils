/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_ENVELOPE_H
# define _MAILUTILS_ENVELOPE_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" { /*}*/
#endif

int mu_envelope_create (envelope_t *, void *);
void mu_envelope_destroy (envelope_t *, void *);
void * mu_envelope_get_owner (envelope_t);
int mu_envelope_set_sender (envelope_t, 
                         int (*_sender) (envelope_t, char *, size_t, size_t*), 
                         void *);
int mu_envelope_sender (envelope_t, char *, size_t, size_t *);
int mu_envelope_set_date (envelope_t, 
                       int (*_date) (envelope_t, char *, size_t , size_t *), 
                       void *);
int mu_envelope_date (envelope_t, char *, size_t, size_t *);

#ifdef __cplusplus
}
#endif
#endif /* _MAILUTILS_ENVELOPE_H */
