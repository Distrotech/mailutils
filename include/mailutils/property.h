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

#ifndef _MAILUTILS_PROPERTY_H
#define _MAILUTILS_PROPERTY_H

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

struct _property;
typedef struct _property *property_t;

extern int property_create   __P ((property_t *, void *));
extern void property_destroy __P ((property_t *, void *));
extern void *property_get_owner __P ((property_t));

extern int property_set_value   __P ((property_t, const char *, const void *));
extern int property_get_value  __P ((property_t, const char *, void **));
extern int property_set_set_value __P ((property_t, int (*_set_value)
					__P ((property_t , const char *,
					      const void *)), void *));
extern int property_set_get_value __P ((property_t , int (*_get_value)
					__P ((property_t, const char *,
					      void **)), void *owner));

extern int property_set  __P ((property_t, const char *));
extern int property_unset  __P ((property_t, const char *));
extern int property_is_set  __P ((property_t, const char *));

extern int property_set_int __P ((property_t, const char *, int));
extern int property_get_int __P ((property_t, const char *));

extern int property_set_long __P ((property_t, const char *, long));
extern long property_get_long __P ((property_t, const char *));

extern int property_set_pointer __P ((property_t, const char *, void *));
extern void *property_get_pointer __P ((property_t, const char *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_PROPERTY_H */
