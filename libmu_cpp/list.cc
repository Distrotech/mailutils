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

#include <mailutils/cpp/list.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// List
//

List :: List ()
{
  int status = list_create (&mu_list);
  if (status)
    throw Exception ("List::List", status);
}

List :: List (const list_t lst)
{
  if (lst == 0)
    throw Exception ("List::List", EINVAL);

  this->mu_list = lst;
}

List :: ~List ()
{
  list_destroy (&mu_list);
}

void
List :: Append (void* item)
{
  int status = list_append (mu_list, item);
  if (status)
    throw Exception ("List::Append", status);
}

void
List :: Prepend (void* item)
{
  int status = list_prepend (mu_list, item);
  if (status)
    throw Exception ("List::Prepend", status);
}

void
List :: Insert (void* item, void* new_item)
{
  int status = list_insert (mu_list, item, new_item);
  if (status)
    throw Exception ("List::Insert", status);
}

void
List :: Remove (void* item)
{
  int status = list_remove (mu_list, item);
  if (status)
    throw Exception ("List::Remove", status);
}

void
List :: Replace (void* old_item, void* new_item)
{
  int status = list_replace (mu_list, old_item, new_item);
  if (status)
    throw Exception ("List::Replace", status);
}

void
List :: Get (size_t index, void** pitem)
{
  int status = list_get (mu_list, index, pitem);
  if (status)
    throw Exception ("List::Get", status);
}

void*
List :: Get (size_t index)
{
  void* pitem;

  int status = list_get (mu_list, index, &pitem);
  if (status)
    throw Exception ("List::Get", status);

  return pitem;
}

void*
List :: operator [] (size_t index)
{
  return this->Get (index);
}

void
List :: ToArray (void** array, size_t count, size_t* pcount)
{
  int status = list_to_array (mu_list, array, count, pcount);
  if (status)
    throw Exception ("List::ToArray", status);
}

void
List :: Locate (void* item, void** ret_item)
{
  int status = list_locate (mu_list, item, ret_item);
  if (status)
    throw Exception ("List::Locate", status);
}

bool
List :: IsEmpty ()
{
  return (bool) list_is_empty (mu_list);
}

size_t
List :: Count ()
{
  size_t count = 0;

  int status = list_count (mu_list, &count);
  if (status)
    throw Exception ("List::Count", status);

  return count;
}

void
List :: Do (list_action_t* action, void* cbdata)
{
  int status = list_do (mu_list, action, cbdata);
  if (status)
    throw Exception ("List::Do", status);
}

list_comparator_t
List :: SetComparator (list_comparator_t comp)
{
  return list_set_comparator (mu_list, comp);
}

void
List :: SetDestroyItem (void (*destroy_item) (void *item))
{
  int status = list_set_destroy_item (mu_list, destroy_item);
  if (status)
    throw Exception ("List::SetDestroyItem", status);
}

