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

#include <mailutils/cpp/message.h>
#include <mailutils/cpp/header.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// Message
//

Message :: Message ()
{
}

Message :: Message (const message_t msg)
{
  if (msg == 0)
    throw Exception ("Message::Message", EINVAL);

  this->msg = msg;
}

Header&
Message :: GetHeader ()
{
  header_t c_hdr;

  int status = message_get_header (msg, &c_hdr);
  if (status)
    throw Exception ("Message::GetHeader", status);

  return *new Header (c_hdr);
}

