/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2008, 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_ALLOC_H
#define _MAILUTILS_ALLOC_H

#include <mailutils/types.h>

# ifndef MU_ATTRIBUTE_NORETURN
#  define MU_ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
# endif

extern void (*mu_alloc_die_hook) (void);
void mu_alloc_die (void) MU_ATTRIBUTE_NORETURN;
void *mu_alloc (size_t);
void *mu_calloc (size_t, size_t);
void *mu_zalloc (size_t size);
void *mu_realloc (void *, size_t);
char *mu_strdup (const char *);
void *mu_2nrealloc (void *, size_t *, size_t);

#endif
