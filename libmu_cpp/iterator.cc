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

#include <mailutils/cpp/iterator.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Iterator
//

Iterator :: Iterator (const List& lst)
{
  int status = list_get_iterator (lst.mu_list, &mu_iter);
  if (status)
    throw Exception ("Iterator::Iterator", status);

  this->pList = (List*) &lst;
}

Iterator :: Iterator (const iterator_t iter)
{
  if (iter == 0)
    throw Exception ("Iterator::Iterator", EINVAL);

  this->mu_iter = iter;
  this->pList = 0;
}

Iterator :: ~Iterator ()
{
  iterator_destroy (&mu_iter);
}

void
Iterator :: First ()
{
  iterator_first (mu_iter);
}

void
Iterator :: Next ()
{
  iterator_next (mu_iter);
}

Iterator&
Iterator :: operator ++ (int)
{
  iterator_next (mu_iter);
  return *this;
}

void
Iterator :: Current (void** pitem)
{
  int status = iterator_current (mu_iter, pitem);
  if (status)
    throw Exception ("Iterator::Current", status);
}

void*
Iterator :: Current ()
{
  void* pitem;

  int status = iterator_current (mu_iter, &pitem);
  if (status)
    throw Exception ("Iterator::Current", status);

  return pitem;
}

bool
Iterator :: IsDone ()
{
  return (bool) iterator_is_done (mu_iter);
}

List&
Iterator :: GetList ()
{
  if (!pList)
    throw Exception ("Iterator::GetList", ENOTSUP);
  return *pList;
}

void
Iterator :: Dup (Iterator*& piter, const Iterator& orig)
{
  iterator_t iter;

  int status = iterator_dup (&iter, orig.mu_iter);
  if (status)
    throw Exception ("Iterator::Dup", status);

  piter->mu_iter = iter;
}

