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

#ifndef _ADDRESS_H
#define _ADDRESS_H

#include <iostream>
#include <mailutils/address.h>
#include <mailutils/cpp/error.h>

namespace mailutils
{

class Address
{
 private:
  char buf[256];

 protected:
  address_t addr;

  friend class Mailer;

 public:
  Address (const std::string&);
  Address (const address_t);
  ~Address ();

  size_t GetCount ();
  bool IsGroup (size_t);

  std::string GetEmail (size_t);
  std::string GetLocalPart (size_t);
  std::string GetDomain (size_t);
  std::string GetPersonal (size_t);
  std::string GetComments (size_t);
  std::string GetRoute (size_t);

  // Address Exceptions
  class EInval : public Exception {
  public:
    EInval (const std::string& m, int s) : Exception (m, s) {}
  };

  class ENoent : public Exception {
  public:
    ENoent (const std::string& m, int s) : Exception (m, s) {}
  };
};

}

#endif // not _ADDRESS_H

