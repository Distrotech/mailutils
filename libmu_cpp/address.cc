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

#include <mailutils/cpp/address.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Address
//

Address :: Address (const std::string& str)
{
  int status = address_create (&addr, str.c_str ());
  if (status)
    throw Exception ("Address::Address", status);
}

Address :: Address (const address_t addr)
{
  if (addr == 0)
    throw Exception ("Address::Address", EINVAL);

  this->addr = addr;
}

Address :: ~Address ()
{
  address_destroy (&addr);
}

bool
Address :: IsGroup (size_t n)
{
  int isgroup;
  int status = address_is_group (addr, n, &isgroup);
  if (status == EINVAL)
    throw Address::EInval ("Address::IsGroup", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::IsGroup", status);

  return (bool) isgroup;
}

size_t
Address :: GetCount ()
{
  size_t count;
  address_get_count (addr, &count);
  return count;
}

std::string
Address :: GetEmail (size_t n)
{
  int status = address_get_email (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::GetEmail", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::GetEmail", status);

  return std::string (buf);
}

std::string
Address :: GetLocalPart (size_t n)
{
  int status = address_get_local_part (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::GetLocalPart", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::GetLocalPart", status);

  return std::string (buf);
}

std::string
Address :: GetDomain (size_t n)
{
  int status = address_get_domain (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::GetDomain", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::GetDomain", status);

  return std::string (buf);
}

std::string
Address :: GetPersonal (size_t n)
{
  int status = address_get_personal (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::GetPersonal", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::GetPersonal", status);

  return std::string (buf);
}

std::string
Address :: GetComments (size_t n)
{
  int status = address_get_comments (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::GetComments", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::GetComments", status);

  return std::string (buf);
}

std::string
Address :: GetRoute (size_t n)
{
  int status = address_get_route (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::GetRoute", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::GetRoute", status);

  return std::string (buf);
}

