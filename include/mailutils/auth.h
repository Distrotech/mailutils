/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2007 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_AUTH_H
#define _MAILUTILS_AUTH_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  mu_ticket_create          (mu_ticket_t *, void *owner);
extern void mu_ticket_destroy         (mu_ticket_t *, void *owner);
extern int  mu_ticket_set_destroy     (mu_ticket_t, void (*) (mu_ticket_t), void *);
extern void *mu_ticket_get_owner      (mu_ticket_t);

extern int mu_ticket_set_pop          (mu_ticket_t, 
                                    int (*_pop) (mu_ticket_t, mu_url_t, const char *, char **), void *);
extern int mu_ticket_pop              (mu_ticket_t, mu_url_t, const char *, char **);
extern int mu_ticket_set_data         (mu_ticket_t, void *, void *owner);
extern int mu_ticket_get_data         (mu_ticket_t, void **);

extern int mu_authority_create           (mu_authority_t *, mu_ticket_t, void *);
extern void mu_authority_destroy         (mu_authority_t *, void *);
extern void *mu_authority_get_owner      (mu_authority_t);
extern int mu_authority_set_ticket       (mu_authority_t, mu_ticket_t);
extern int mu_authority_get_ticket       (mu_authority_t, mu_ticket_t *);
extern int mu_authority_authenticate     (mu_authority_t);
extern int mu_authority_set_authenticate (mu_authority_t, 
                                            int (*_authenticate) (mu_authority_t), void *);

extern int mu_authority_create_null      (mu_authority_t *pauthority, void *owner);

extern int  mu_wicket_create       (mu_wicket_t *, const char *);
extern void mu_wicket_destroy      (mu_wicket_t *);
extern int  mu_wicket_set_filename (mu_wicket_t, const char *);
extern int  mu_wicket_get_filename (mu_wicket_t, char *, size_t, size_t *);
extern int  mu_wicket_set_ticket   (mu_wicket_t, 
                                 int (*) (mu_wicket_t, const char *,
			         const char *, mu_ticket_t *));
extern int  mu_wicket_get_ticket   (mu_wicket_t, mu_ticket_t *, const char *, const char *);

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_AUTH_H */
