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

#ifndef _URL_H
#define _URL_H

#include <iostream>
#include <string>
#include <mailutils/url.h>

namespace mailutils
{

class Url
{
 private:
  char buf[1024];

 protected:
  url_t url;

 public:
  Url (const std::string&);
  Url (const char*);
  Url (const url_t);
  ~Url ();

  void Parse ();
  long GetPort ();
  std::string GetScheme ();
  std::string GetUser ();
  std::string GetPasswd ();
  std::string GetAuth ();
  std::string GetHost ();
  std::string GetPath ();
  std::string GetQuery ();
};

}

#endif // not _URL_H

