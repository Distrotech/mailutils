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

#ifndef _MAILCAP_H
#define _MAILCAP_H

#include <iostream>
#include <mailutils/mailcap.h>
#include <mailutils/cpp/stream.h>

namespace mailutils
{

class MailcapEntry
{
 private:
  char buf[256];

 protected:
  mu_mailcap_entry_t entry;

 public:
  MailcapEntry (mu_mailcap_entry_t);

  size_t FieldsCount ();
  std::string GetTypeField ();
  std::string GetViewCommand ();
  std::string GetField (size_t);
};

class Mailcap
{
 protected:
  mu_mailcap_t mailcap;

 public:
  Mailcap (const Stream&);
  Mailcap (const mu_mailcap_t);
  ~Mailcap ();

  size_t GetCount ();
  MailcapEntry& GetEntry (size_t);
};

}

#endif // not _MAILCAP_H

