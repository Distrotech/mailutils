/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

#ifndef _OBSERVER0_H
#define _OBSERVER0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/observer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _observer
{
  int flags;
  void *owner;
  int (*_action)  __P ((observer_t, size_t));
  int (*_destroy) __P ((observer_t));
};

struct _observable
{
  void *owner;
  list_t list;
};

struct _event
{
  size_t type;
  observer_t observer;
};

typedef struct _event *event_t;



#ifdef __cplusplus
}
#endif

#endif /* _OBSERVER0_H */
