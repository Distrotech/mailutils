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

#include <mailutils/cpp/filter.h>

using namespace mailutils;

//
// FilterStream
//

void
FilterStream :: Create (Stream& transport,
			const std::string& code,
			int mode, int flag)
{
  int status = filter_create (&this->stm,
			      transport.stm,
			      code.c_str (),
			      mode, flag);
  if (status)
    throw Exception ("FilterStream::Create", status);
  this->input = new Stream (transport);
}

void
FilterStream :: IconvCreate (Stream& transport,
			     const std::string& fromcode,
			     const std::string& tocode,
			     int flags,
			     enum mu_iconv_fallback_mode fallback_mode)
{
  int status = filter_iconv_create (&this->stm, transport.stm,
				    fromcode.c_str (),
				    tocode.c_str (),
				    flags, fallback_mode);
  if (status)
    throw Exception ("FilterStream::IconvCreate", status);
  this->input = new Stream (transport);
}

