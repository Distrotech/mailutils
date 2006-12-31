/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA
*/

#ifndef _MUCPP_LIST_H
#define _MUCPP_LIST_H

#include <iostream>
#include <mailutils/list.h>

typedef int mu_list_action_t (void*, void*);
typedef int (*mu_list_comparator_t) (const void*, const void*);

namespace mailutils
{

class List
{
 protected:
  mu_list_t mu_list;

  friend class Iterator;

 public:
  List ();
  List (const mu_list_t);
  ~List ();

  void append (void*);
  void prepend (void*);
  void insert (void*, void*, int);
  void remove (void*);
  void replace (void*, void*);

  void  get (size_t, void**);
  void* get (size_t);
  void* operator [] (size_t);

  void toArray (void**, size_t, size_t*);
  void locate (void*, void**);

  void apply (mu_list_action_t*, void*);
  mu_list_comparator_t setComparator (mu_list_comparator_t);
  void setDestroyItem (void (*mu_destoy_item) (void *));

  bool isEmpty ();
  size_t count ();
};

}

#endif // not _MUCPP_LIST_H

