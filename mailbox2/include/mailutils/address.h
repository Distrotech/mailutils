/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_ADDRESS_H
#define _MAILUTILS_ADDRESS_H

#include <sys/types.h>
#include <mailutils/mu_features.h>
#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  address_create   __P ((address_t *, const char *));

extern int  address_ref      __P ((address_t));
extern void address_destroy  __P ((address_t *));

extern int address_get_email
	__P ((address_t, size_t, char *, size_t, size_t *));
extern int address_get_local_part
	__P ((address_t, size_t, char *, size_t, size_t *));
extern int address_get_domain
	__P ((address_t, size_t, char *, size_t, size_t *));
extern int address_get_personal
	__P ((address_t, size_t, char *, size_t, size_t *));
extern int address_get_comments
	__P ((address_t, size_t, char *, size_t, size_t *));
extern int address_get_route
	__P ((address_t, size_t, char *, size_t, size_t *));

extern int address_is_group
	__P ((address_t, size_t, int*));

extern int address_to_string __P ((address_t, char *, size_t, size_t *));
extern int address_get_count __P ((address_t, size_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ADDRESS_H */
