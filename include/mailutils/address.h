/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005  Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_ADDRESS_H
#define _MAILUTILS_ADDRESS_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int address_create   (address_t *, const char *);
extern int address_createv  (address_t *, const char *v[], size_t);
extern void address_destroy (address_t *);

extern address_t address_dup (address_t src);

/* Set FROM to null, after adding its addresses to TO. */
extern int address_concatenate (address_t to, address_t* from);

extern int address_get_nth
        (address_t addr, size_t no, address_t *pret);
extern int address_get_email
	(address_t, size_t, char *, size_t, size_t *);
extern int address_get_local_part
	(address_t, size_t, char *, size_t, size_t *);
extern int address_get_domain
	(address_t, size_t, char *, size_t, size_t *);
extern int address_get_personal
	(address_t, size_t, char *, size_t, size_t *);
extern int address_get_comments
	(address_t, size_t, char *, size_t, size_t *);
extern int address_get_route
	(address_t, size_t, char *, size_t, size_t *);

extern int address_aget_email
	(address_t, size_t, char **);
extern int address_aget_local_part
       (address_t addr, size_t no, char **buf);
extern int address_aget_domain
       (address_t addr, size_t no, char **buf);
extern int address_aget_personal
       (address_t addr, size_t no, char **buf);

extern int address_is_group
	(address_t, size_t, int*);

extern int address_to_string (address_t, char *, size_t, size_t *);
extern int address_get_count (address_t, size_t *);
extern int address_get_group_count (address_t, size_t *);
extern int address_get_email_count (address_t, size_t *);
extern int address_get_unix_mailbox_count (address_t, size_t *);

extern int address_contains_email (address_t addr, const char *email);
extern int address_union (address_t *a, address_t b);
  
extern size_t address_format_string (address_t addr, char *buf, size_t buflen);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ADDRESS_H */
