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

#ifndef _FILTER_H
#define _FILTER_H

#include <iostream>
#include <mailutils/filter.h>
#include <mailutils/cpp/stream.h>

namespace mailutils
{

class FilterStream : public Stream
{
 private:
  Stream *input;

 public:
  void Create (Stream&, const std::string&, int, int);
  void IconvCreate (Stream&, const std::string&,
		    const std::string&, int,
		    enum mu_iconv_fallback_mode);
};

}

#endif // not _FILTER_H

