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

#ifndef _MUCPP_MESSAGE_H
#define _MUCPP_MESSAGE_H

#include <mailutils/message.h>
#include <mailutils/cpp/header.h>

namespace mailutils
{

class Message
{
 protected:
  mu_message_t msg;

  friend class Mailer;

 public:
  Message ();
  Message (const mu_message_t);

  Header& getHeader ();
};

}

#endif // not _MUCPP_MESSAGE_H

