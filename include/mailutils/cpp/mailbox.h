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

#ifndef _MAILBOX_H
#define _MAILBOX_H

#include <iostream>
#include <mailutils/mailbox.h>
#include <mailutils/cpp/message.h>

namespace mailutils
{

class MailboxBase
{
 protected:
  mailbox_t mbox;

 public:
  void Open (int);
  void Close ();

  size_t MessagesCount ();
  Message& GetMessage (size_t);
  Message& operator [] (size_t);
};

class Mailbox : public MailboxBase
{
 public:
  Mailbox (const std::string&);
  Mailbox (const mailbox_t);
  ~Mailbox ();
};

class MailboxDefault : public MailboxBase
{
 public:
  MailboxDefault (const std::string&);
  MailboxDefault (const mailbox_t);
  ~MailboxDefault ();
};

}

#endif /* not _MAILBOX_H */

