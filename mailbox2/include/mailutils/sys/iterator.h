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

#ifndef _MAILUTILS_SYS_ITERATOR_H
#define _MAILUTILS_SYS_ITERATOR_H

#include <mailutils/iterator.h>

__MAILUTILS_BEGIN_DECLS

struct _iterator_vtable
{
  /* Base */
  int (*add_ref) __P ((iterator_t));
  int (*release) __P ((iterator_t));
  int (*destroy) __P ((iterator_t));

  int (*first)   __P ((iterator_t));
  int (*next)    __P ((iterator_t));
  int (*current) __P ((iterator_t, void *));
  int (*is_done) __P ((iterator_t));
};

struct _iterator
{
  struct _iterator_vtable *vtable;
};

/* Use macros for the implentation.  */
#define iterator_add_ref(i)    ((i)->vtable->add_ref)(i)
#define iterator_release(i)    ((i)->vtable->release)(i)
#define iterator_destroy(i)    ((i)->vtable->destroy)(i)

#define iterator_first(i)      ((i)->vtable->first)(i)
#define iterator_next(i)       ((i)->vtable->next)(i)
#define iterator_current(i,a)  ((i)->vtable->current)(i,a)
#define iterator_is_done(i)    ((i)->vtable->is_done)(i)

__MAILUTILS_END_DECLS

#endif /* _MAILUTILS_SYS_ITERATOR_H */
