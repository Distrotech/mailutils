/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2005, 2006, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

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

extern int mu_address_create   (mu_address_t *, const char *);
extern int mu_address_createv  (mu_address_t *, const char *v[], size_t);
extern void mu_address_destroy (mu_address_t *);

extern mu_address_t mu_address_dup (mu_address_t src);

/* Set FROM to null, after adding its addresses to TO. */
extern int mu_address_concatenate (mu_address_t to, mu_address_t* from);

extern int mu_address_get_nth
        (mu_address_t addr, size_t no, mu_address_t *pret);

extern int mu_address_set_email (mu_address_t, size_t, const char *);
extern int mu_address_sget_email
	(mu_address_t, size_t, const char **);
extern int mu_address_get_email
	(mu_address_t, size_t, char *, size_t, size_t *);
extern int mu_address_aget_email
	(mu_address_t, size_t, char **);

extern int mu_address_set_comments (mu_address_t, size_t, const char *);
extern int mu_address_sget_comments
	(mu_address_t, size_t, const char **);
extern int mu_address_get_comments
	(mu_address_t, size_t, char *, size_t, size_t *);
extern int mu_address_aget_comments
	(mu_address_t, size_t, char **);

extern int mu_address_set_local_part (mu_address_t, size_t, const char *);
extern int mu_address_sget_local_part
	(mu_address_t, size_t, const char **);
extern int mu_address_get_local_part
	(mu_address_t, size_t, char *, size_t, size_t *);
extern int mu_address_aget_local_part
	(mu_address_t, size_t, char **);

extern int mu_address_set_personal (mu_address_t, size_t, const char *);
extern int mu_address_sget_personal
	(mu_address_t, size_t, const char **);
extern int mu_address_get_personal
	(mu_address_t, size_t, char *, size_t, size_t *);
extern int mu_address_aget_personal
	(mu_address_t, size_t, char **);

extern int mu_address_set_domain (mu_address_t, size_t, const char *);
extern int mu_address_sget_domain
	(mu_address_t, size_t, const char **);
extern int mu_address_get_domain
	(mu_address_t, size_t, char *, size_t, size_t *);
extern int mu_address_aget_domain
	(mu_address_t, size_t, char **);

extern int mu_address_set_route (mu_address_t, size_t, const char *);
extern int mu_address_sget_route
	(mu_address_t, size_t, const char **);
extern int mu_address_get_route
	(mu_address_t, size_t, char *, size_t, size_t *);
extern int mu_address_aget_route
	(mu_address_t, size_t, char **);

extern int mu_address_is_group
	(mu_address_t, size_t, int*);

extern int mu_address_to_string (mu_address_t, char *, size_t, size_t *);
extern int mu_address_get_count (mu_address_t, size_t *);
extern int mu_address_get_group_count (mu_address_t, size_t *);
extern int mu_address_get_email_count (mu_address_t, size_t *);
extern int mu_address_get_unix_mailbox_count (mu_address_t, size_t *);

extern int mu_address_contains_email (mu_address_t addr, const char *email);
extern int mu_address_union (mu_address_t *a, mu_address_t b);
  
extern size_t mu_address_format_string (mu_address_t addr, char *buf, size_t buflen);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_ADDRESS_H */
