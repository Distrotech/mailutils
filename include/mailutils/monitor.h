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

#ifndef _MAILUTILS_MONITOR_H
#define _MAILUTILS_MONITOR_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _monitor
{
  void *data;
  void *owner;
  int allocated;
  int flags;
};
typedef struct _monitor *monitor_t;

#define MU_MONITOR_PTHREAD 0
#define MU_MONITOR_INITIALIZER {0, 0, 0, 0}


extern int monitor_create      __P ((monitor_t *, int flags, void *owner));
extern void monitor_destroy    __P ((monitor_t *, void *owner));
extern void *monitor_get_owner __P ((monitor_t));

extern int monitor_rdlock      __P ((monitor_t));
extern int monitor_wrlock      __P ((monitor_t));
extern int monitor_unlock      __P ((monitor_t));
extern int monitor_wait        __P ((monitor_t));
extern int monitor_notify      __P ((monitor_t));

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_MONITOR_H */
