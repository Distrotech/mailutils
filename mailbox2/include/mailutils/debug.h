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

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifndef _MAILUTILS_DEBUG_H
#define _MAILUTILS_DEBUG_H

#include <sys/types.h>
#include <mailutils/mu_features.h>
#include <mailutils/types.h>
#include <mailutils/stream.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MU_DEBUG_TRACE 1
#define MU_DEBUG_PROT  2

extern int  mu_debug_ref       __P ((mu_debug_t));
extern void mu_debug_destroy   __P ((mu_debug_t *));
extern int  mu_debug_set_level __P ((mu_debug_t, unsigned int));
extern int  mu_debug_get_level __P ((mu_debug_t, unsigned int *));
extern int  mu_debug_print     __P ((mu_debug_t, unsigned int, const char *));

extern int  mu_debug_stream_create   __P ((mu_debug_t *, stream_t, int));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_DEBUG_H */
