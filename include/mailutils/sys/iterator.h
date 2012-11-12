/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2004, 2007, 2010-2012 Free Software
   Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_ITERATOR_H
# define _MAILUTILS_SYS_ITERATOR_H

# include <mailutils/iterator.h>

# ifdef __cplusplus
extern "C" {
# endif

struct _mu_iterator
{
  struct _mu_iterator *next_itr; /* Next iterator in the chain */
  void *owner;                /* Object whose contents is being iterated */
  int is_advanced;            /* Is the iterator already advanced */

  int (*dup) (void **ptr, void *owner);
  int (*destroy) (mu_iterator_t itr, void *owner);
  int (*first) (void *owner);
  int (*next) (void *owner);
  int (*getitem) (void *owner, void **pret, const void **pkey);
  int (*delitem) (void *owner, void *item);
  int (*finished_p) (void *owner);
  int (*itrctl) (void *owner, enum mu_itrctl_req req, void *arg);
  void *(*dataptr) (void *);
};

# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_ITERATOR_H */
