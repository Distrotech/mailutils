/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#ifndef _LIST_H
#define _LIST_H

#include <iostream>
#include <mailutils/list.h>

typedef int list_action_t (void*, void*);
typedef int (*list_comparator_t) (const void*, const void*);

namespace mailutils
{

class List
{
 protected:
  list_t mu_list;

  friend class Iterator;

 public:
  List ();
  List (const list_t);
  ~List ();

  void Append (void*);
  void Prepend (void*);
  void Insert (void*, void*);
  void Remove (void*);
  void Replace (void*, void*);

  void  Get (size_t, void**);
  void* Get (size_t);
  void* operator [] (size_t);

  void ToArray (void**, size_t, size_t*);
  void Locate (void*, void**);

  void Do (list_action_t*, void*);
  list_comparator_t SetComparator (list_comparator_t);
  void SetDestroyItem (void (*destoy_item) (void *));

  bool IsEmpty ();
  size_t Count ();
};

}

#endif // not _LIST_H

