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

#ifndef _MAILUTILS_LIST_H
#define _MAILUTILS_LIST_H

#include <sys/types.h>
#include <mailutils/mu_features.h>
#include <mailutils/types.h>
#include <mailutils/iterator.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int  mu_list_create       __P ((mu_list_t *));
extern int  mu_list_ref          __P ((mu_list_t));
extern void mu_list_destroy      __P ((mu_list_t *));
extern int  mu_list_append       __P ((mu_list_t, void *));
extern int  mu_list_prepend      __P ((mu_list_t, void *));
extern int  mu_list_is_empty     __P ((mu_list_t));
extern int  mu_list_count        __P ((mu_list_t, size_t *));
extern int  mu_list_remove       __P ((mu_list_t, void *));
extern int  mu_list_get          __P ((mu_list_t, size_t, void **));
extern int  mu_list_get_iterator __P ((mu_list_t, iterator_t *));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_LIST_H */
