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

#ifndef _LIST0_H
#define _LIST0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>

#include <mailutils/list.h>
#include <mailutils/monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

struct list_data
{
  void *item;
  struct list_data *next;
  struct list_data *prev;
};

struct _list
{
  struct list_data head;
  size_t count;
  monitor_t monitor;
  list_comparator_t comp;
};


#ifdef __cplusplus
}
#endif

#endif /* _LIST0_H */
