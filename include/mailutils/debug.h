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

#ifndef _MAILUTILS_DEBUG_H
#define _MAILUTILS_DEBUG_H

#include <sys/types.h>
#include <stdarg.h>

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

struct _debug;
typedef struct _debug* mu_debug_t;

#define MU_DEBUG_ERROR 1
#define MU_DEBUG_TRACE 2
#define MU_DEBUG_PROT  3

extern int mu_debug_create    __P ((mu_debug_t *, void *owner));
extern void mu_debug_destroy  __P ((mu_debug_t *, void *owner));
extern void * mu_debug_get_owner __P ((mu_debug_t));
extern int mu_debug_set_level __P ((mu_debug_t, size_t level));
extern int mu_debug_get_level __P ((mu_debug_t, size_t *plevel));
extern int mu_debug_print     __P ((mu_debug_t debug, size_t level,
				 const char *format, ...));
extern int mu_debug_printv    __P ((mu_debug_t debug, size_t level,
				 const char *format, va_list argp));
extern int mu_debug_set_print __P ((mu_debug_t, int (*_print) __P ((mu_debug_t, size_t level, const char *, va_list)), void *owner));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_DEBUG_H */
