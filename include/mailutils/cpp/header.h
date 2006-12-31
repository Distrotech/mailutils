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

#ifndef _MUCPP_HEADER_H
#define _MUCPP_HEADER_H

#include <iostream>
#include <string>
#include <mailutils/header.h>

namespace mailutils
{

class Header
{
 protected:
  mu_header_t hdr;

 public:
  Header ();
  Header (const mu_header_t);

  std::string getValue (const std::string&);
  std::string operator [] (const std::string&);
};

}

#endif // not _MUCPP_HEADER_H

