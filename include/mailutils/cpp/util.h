/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MUCPP_UTIL_H
#define _MUCPP_UTIL_H

#include <string>
#include <mailutils/filter.h>
#include <mailutils/util.h>

namespace mailutils
{

int set_user_email (const std::string& email);
int set_user_email_domain (const std::string& domain);
std::string tempname ();
std::string tempname (const std::string& tmpdir);

}

#endif // not _MUCPP_UTIL_H

