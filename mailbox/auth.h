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

#ifndef _AUTH_H
#define _AUTH_H

#include <sys/types.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef _cpluscplus
extern "C" {
#endif

/* forward declaration */
struct _auth;
typedef struct _auth *auth_t;

extern int auth_init       __P ((auth_t *));
extern void auth_destroy   __P ((auth_t *));

/* login/passwd */
extern int auth_get_login  __P ((auth_t, char *login,
				  size_t len, size_t *n));
extern int auth_set_login  __P ((auth_t, const char *login, size_t len));

extern int auth_get_passwd __P ((auth_t, char *passwd,
				  size_t len, size_t *n));
extern int auth_set_passwd __P ((auth_t, const char *passwd, size_t len));

/* owner group mode*/
extern int auth_get_owner  __P ((auth_t, uid_t *uid));
extern int auth_set_owner  __P ((auth_t, uid_t uid));
extern int auth_get_group  __P ((auth_t, gid_t *gid));
extern int auth_set_group  __P ((auth_t, gid_t gid));
extern int auth_get_mode  __P ((auth_t, mode_t *mode));
extern int auth_set_mode  __P ((auth_t, mode_t mode));

#ifdef _cpluscplus
}
#endif

#endif /* _AUTH_H */
