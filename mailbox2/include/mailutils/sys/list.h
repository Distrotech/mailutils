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

#ifndef _MAILUTILS_SYS_LIST_H
#define _MAILUTILS_SYS_LIST_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>

#include <mailutils/list.h>
#include <mailutils/refcount.h>
#include <mailutils/sys/iterator.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mu_list_data
{
  void *item;
  struct mu_list_data *next;
  struct mu_list_data *prev;
};

struct _list
{
  struct mu_list_data head;
  size_t count;
  size_t index;
  mu_refcount_t refcount;
};

struct l_iterator
{
  struct _iterator base;
  mu_list_t list;
  struct mu_list_data *current;
  mu_refcount_t refcount;
};


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_LIST_H */
