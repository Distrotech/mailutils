/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006, 2007 Free Software Foundation, Inc.

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

Message :: Message (const mu_message_t msg)
{
  if (msg == 0)
    throw Exception ("Message::Message", EINVAL);

  this->msg = msg;
}

Header&
Message :: getHeader ()
{
  mu_header_t c_hdr;

  int status = mu_message_get_header (msg, &c_hdr);
  if (status)
    throw Exception ("Message::getHeader", status);

  return *new Header (c_hdr);
}

