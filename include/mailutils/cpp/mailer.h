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

#ifndef _MAILER_H
#define _MAILER_H

#include <iostream>
#include <string>
#include <mailutils/mailer.h>
#include <mailutils/cpp/message.h>
#include <mailutils/cpp/address.h>

namespace mailutils
{

class Mailer
{
 protected:
  mailer_t mailer;

 public:
  Mailer (const std::string&);
  Mailer (const mailer_t);
  ~Mailer ();

  void Open (int);
  void Close ();
  void SendMessage (const Message&, const Address&, const Address&);
};

}

#endif // not _MAILER_H

