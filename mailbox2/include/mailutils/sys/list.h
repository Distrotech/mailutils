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

#ifndef _MAILUTILS_SYS_LIST_H
#define _MAILUTILS_SYS_LIST_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>

#include <mailutils/list.h>
#include <mailutils/monitor.h>
#include <mailutils/sys/iterator.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

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
  size_t index;
  monitor_t lock;
};

struct l_iterator
{
  struct _iterator base;
  unsigned int ref;
  list_t list;
  struct list_data *current;
  monitor_t lock;
};


#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_LIST_H */
