/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006, 2007, 2009 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA
*/

#include <mailutils/cpp/header.h>

using namespace mailutils;

//
// Header
//

Header :: Header ()
{
  this->hdr = NULL;
}

Header :: Header (const mu_header_t hdr)
{
  if (hdr == 0)
    throw Exception ("Header::Header", EINVAL);

  this->hdr = hdr;
}

std::string
Header :: get_value (const std::string& name)
{
  char* c_val;

  int status = mu_header_aget_value (hdr, name.c_str (), &c_val);
  if (status)
    throw Exception ("Header::get_value", status);

  std::string val (c_val);
  free (c_val);
  return val;
}

std::string
Header :: get_value (const std::string& name, const std::string& def)
{
  char* c_val;

  int status = mu_header_aget_value (hdr, name.c_str (), &c_val);
  if (status == MU_ERR_NOENT)
    return std::string (def);
  else if (status)
    throw Exception ("Header::get_value", status);

  std::string val (c_val);
  free (c_val);
  return val;
}

size_t
Header :: size ()
{
  size_t c_size;
  int status = mu_header_size (hdr, &c_size);
  if (status)
    throw Exception ("Header::size", status);
  return c_size;
}

size_t
Header :: lines ()
{
  size_t c_lines;
  int status = mu_header_lines (hdr, &c_lines);
  if (status)
    throw Exception ("Header::lines", status);
  return c_lines;
}

