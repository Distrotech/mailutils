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

#include <mailutils/cpp/header.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Header
//

Header :: Header ()
{
}

Header :: Header (const header_t hdr)
{
  if (hdr == 0)
    throw Exception ("Header::Header", EINVAL);

  this->hdr = hdr;
}

std::string
Header :: GetValue (const std::string& name)
{
  char* c_val;

  int status = header_aget_value (hdr, name.c_str (), &c_val);
  if (status)
    throw Exception ("Header::GetValue", status);

  std::string val (c_val);
  free (c_val);
  return val;
}

std::string
Header :: operator [] (const std::string& name)
{
  return this->GetValue (name);
}

