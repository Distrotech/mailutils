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

extern int auth_init          __P ((auth_t *, void *owner));
extern void auth_destroy      __P ((auth_t *, void *owner));

extern int auth_prologue      __P ((auth_t));
extern int auth_set_prologue __P ((auth_t auth,
				   int (*_prologue) __P ((auth_t)),
				   void *owner));

extern int auth_authenticate  __P ((auth_t));
extern int auth_set_authenticate __P ((auth_t auth,
				       int (*_authenticate) __P ((auth_t)),
				       void *owner));

extern int auth_epilogue      __P ((auth_t));
extern int auth_set_epilogue __P ((auth_t auth,
				   int (*_epilogue) __P ((auth_t)),
				   void *owner));

#ifdef _cpluscplus
}
#endif

#endif /* _AUTH_H */
