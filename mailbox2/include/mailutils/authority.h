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

#ifndef _MAILUTILS_AUTHORITY_H
#define _MAILUTILS_AUTHORITY_H

#include <sys/types.h>
#include <mailutils/ticket.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
struct _authority;
typedef struct _authority *authority_t;

extern int  authority_ref          __P ((authority_t));
extern void authority_destroy      __P ((authority_t *));
extern int  authority_set_ticket   __P ((authority_t, ticket_t));
extern int  authority_get_ticket   __P ((authority_t, ticket_t *));
extern int  authority_authenticate __P ((authority_t));

extern int authority_userpass_create __P ((authority_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_AUTHORITY_H */
