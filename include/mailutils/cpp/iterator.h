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

#ifndef _ITERATOR_H
#define _ITERATOR_H

#include <iostream>
#include <mailutils/iterator.h>
#include <mailutils/cpp/list.h>

namespace mailutils
{

class Iterator
{
 protected:
  iterator_t mu_iter;
  List* pList;

 public:
  Iterator (const List&);
  Iterator (const iterator_t);
  ~Iterator ();

  void Dup (Iterator*&, const Iterator&);
  void First ();
  void Next ();
  Iterator& operator ++ (int);
  void Current (void**);
  void* Current ();
  bool IsDone ();
  List& GetList ();
};

}

#endif // not _ITERATOR_H

