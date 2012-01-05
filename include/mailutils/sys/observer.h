/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2000, 2005, 2007, 2010-2012 Free Software
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

#ifndef _MAILUTILS_SYS_OBSERVER_H
# define _MAILUTILS_SYS_OBSERVER_H

# include <mailutils/observer.h>

# ifdef __cplusplus
extern "C" {
# endif

struct _mu_observer
{
  int flags;
  void *owner;
  int (*_action)  (mu_observer_t, size_t, void *, void *);
  void *_action_data;
  int (*_destroy) (mu_observer_t, void *data);
};

struct _mu_observable
{
  void *owner;
  mu_list_t list;
};

struct _event
{
  size_t type;
  mu_observer_t observer;
};

typedef struct _event *event_t;

# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_OBSERVER_H */
