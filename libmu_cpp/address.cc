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

#include <mailutils/cpp/address.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Address
//

Address :: Address (const std::string& str)
{
  int status = mu_address_create (&addr, str.c_str ());
  if (status)
    throw Exception ("Address::Address", status);
}

Address :: Address (const mu_address_t addr)
{
  if (addr == 0)
    throw Exception ("Address::Address", EINVAL);

  this->addr = addr;
}

Address :: ~Address ()
{
  mu_address_destroy (&addr);
}

bool
Address :: isGroup (size_t n)
{
  int isgroup;
  int status = mu_address_is_group (addr, n, &isgroup);
  if (status == EINVAL)
    throw Address::EInval ("Address::IsGroup", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::IsGroup", status);

  return (bool) isgroup;
}

size_t
Address :: getCount ()
{
  size_t count;
  mu_address_get_count (addr, &count);
  return count;
}

std::string
Address :: getEmail (size_t n)
{
  int status = mu_address_get_email (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::getEmail", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::getEmail", status);

  return std::string (buf);
}

std::string
Address :: getLocalPart (size_t n)
{
  int status = mu_address_get_local_part (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::getLocalPart", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::getLocalPart", status);

  return std::string (buf);
}

std::string
Address :: getDomain (size_t n)
{
  int status = mu_address_get_domain (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::getDomain", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::getDomain", status);

  return std::string (buf);
}

std::string
Address :: getPersonal (size_t n)
{
  int status = mu_address_get_personal (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::getPersonal", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::getPersonal", status);

  return std::string (buf);
}

std::string
Address :: getComments (size_t n)
{
  int status = mu_address_get_comments (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::getComments", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::getComments", status);

  return std::string (buf);
}

std::string
Address :: getRoute (size_t n)
{
  int status = mu_address_get_route (addr, n, buf, sizeof (buf), 0);
  if (status == EINVAL)
    throw Address::EInval ("Address::getRoute", status);
  else if (status == ENOENT)
    throw Address::ENoent ("Address::getRoute", status);

  return std::string (buf);
}

