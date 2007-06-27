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
  mu_address_t addr;

  friend class Mailer;

 public:
  Address (const std::string&);
  Address (const mu_address_t);
  ~Address ();

  size_t getCount ();
  bool isGroup (size_t);

  std::string getEmail (size_t);
  std::string getLocalPart (size_t);
  std::string getDomain (size_t);
  std::string getPersonal (size_t);
  std::string getComments (size_t);
  std::string getRoute (size_t);

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

