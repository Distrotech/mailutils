/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_AUTH_H
#define _MAILUTILS_AUTH_H

#include <sys/types.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef _cplusplus
extern "C" {
#endif

/* forward declaration */
struct _ticket;
typedef struct _ticket *ticket_t;

extern int ticket_create           __P ((ticket_t *, void *owner));
extern void ticket_destroy         __P ((ticket_t *, void *owner));
extern void * ticket_get_owner     __P ((ticket_t));

extern int ticket_set_pop          __P ((ticket_t, int (*_pop) __P ((ticket_t, const char *, char **)), void *));
extern int ticket_pop              __P ((ticket_t, const char *, char **));

extern int ticket_get_type         __P ((ticket_t, char *, size_t, size_t *));
extern int ticket_set_type         __P ((ticket_t, char *));

struct _authority;
typedef struct _authority *authority_t;

extern int authority_create           __P ((authority_t *, ticket_t, void *));
extern void authority_destroy         __P ((authority_t *, void *));
extern void *authority_get_owner      __P ((authority_t));
extern int authority_set_ticket       __P ((authority_t, ticket_t));
extern int authority_get_ticket       __P ((authority_t, ticket_t *));
extern int authority_authenticate     __P ((authority_t));
extern int authority_set_authenticate __P ((authority_t, int (*_authenticate) __P ((authority_t)), void *));

#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_AUTH_H */
