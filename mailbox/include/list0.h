/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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
};


#ifdef __cplusplus
}
#endif

#endif /* _LIST0_H */
