/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_ADDRESS_H
#define _MAILUTILS_ADDRESS_H

#include <sys/types.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

#ifdef __cplusplus
extern "C" {
#endif

struct _address;
typedef struct _address *address_t;

extern int address_create   __P ((address_t *, const char *));

extern int address_add_ref  __P ((address_t));
extern int address_release  __P ((address_t));
extern int address_destroy  __P ((address_t));

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
