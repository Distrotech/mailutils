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

#ifndef _MAILUTILS_LIST_H
#define _MAILUTILS_LIST_H

#include <sys/types.h>
#include <mailutils/iterator.h>

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

struct _list;
typedef struct _list *mu_list_t;

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
