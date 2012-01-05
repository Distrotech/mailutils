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

#ifndef _MAILUTILS_SYS_LIST_H
# define _MAILUTILS_SYS_LIST_H

# include <sys/types.h>

# include <mailutils/list.h>
# include <mailutils/monitor.h>
# include <mailutils/iterator.h>

# ifdef __cplusplus
extern "C" {
# endif

struct list_data
{
  void *item;
  struct list_data *next;
  struct list_data *prev;
};

struct _mu_list
{
  struct list_data head;
  size_t count;
  mu_monitor_t monitor;
  mu_list_comparator_t comp;
  void (*destroy_item) (void *item);
  struct _mu_iterator *itr;
};

#define DESTROY_ITEM(list, elt)			\
  do						\
    {						\
       if ((list)->destroy_item)		\
	 (list)->destroy_item ((elt)->item);	\
    }						\
  while (0)

extern void _mu_list_clear (mu_list_t list);
extern void _mu_list_insert_sublist (mu_list_t list,
				     struct list_data *current,
				     struct list_data *head,
				     struct list_data *tail,
				     size_t count,
				     int insert_before);
extern int _mu_list_insert_item (mu_list_t list, struct list_data *current,
				 void *new_item,
				 int insert_before);

# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_LIST_H */
