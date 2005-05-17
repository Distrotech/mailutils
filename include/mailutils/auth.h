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

extern int  ticket_create          __P ((ticket_t *, void *owner));
extern void ticket_destroy         __P ((ticket_t *, void *owner));
extern int  ticket_set_destroy     __P ((ticket_t, void (*)
					 __PMT ((ticket_t)), void *owner));
extern void *ticket_get_owner      __P ((ticket_t));

extern int ticket_set_pop          __P ((ticket_t, int (*_pop)
					 __PMT ((ticket_t, url_t, const char *, char **)), void *));
extern int ticket_pop              __P ((ticket_t, url_t, const char *, char **));
extern int ticket_set_data         __P ((ticket_t, void *, void *owner));
extern int ticket_get_data         __P ((ticket_t, void **));

extern int authority_create           __P ((authority_t *, ticket_t, void *));
extern void authority_destroy         __P ((authority_t *, void *));
extern void *authority_get_owner      __P ((authority_t));
extern int authority_set_ticket       __P ((authority_t, ticket_t));
extern int authority_get_ticket       __P ((authority_t, ticket_t *));
extern int authority_authenticate     __P ((authority_t));
extern int authority_set_authenticate __P ((authority_t, 
                                            int (*_authenticate) __PMT ((authority_t)), void *));

extern int authority_create_null      __P ((authority_t *pauthority, void *owner));

extern int  wicket_create       __P ((wicket_t *, const char *));
extern void wicket_destroy      __P ((wicket_t *));
extern int  wicket_set_filename __P ((wicket_t, const char *));
extern int  wicket_get_filename __P ((wicket_t, char *, size_t, size_t *));
extern int  wicket_set_ticket   __P ((wicket_t, int (*)
				      __PMT ((wicket_t, const char *,
					      const char *, ticket_t *))));
extern int  wicket_get_ticket   __P ((wicket_t, ticket_t *, const char *, const char *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_AUTH_H */
