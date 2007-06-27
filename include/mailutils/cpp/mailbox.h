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

#ifndef _MUCPP_MAILBOX_H
#define _MUCPP_MAILBOX_H

#include <iostream>
#include <mailutils/mailbox.h>
#include <mailutils/cpp/message.h>

namespace mailutils
{

class MailboxBase
{
 protected:
  mu_mailbox_t mbox;

 public:
  void open (int);
  void close ();

  size_t messagesCount ();
  Message& getMessage (size_t);
  Message& operator [] (size_t);
};

class Mailbox : public MailboxBase
{
 public:
  Mailbox (const std::string&);
  Mailbox (const mu_mailbox_t);
  ~Mailbox ();
};

class MailboxDefault : public MailboxBase
{
 public:
  MailboxDefault (const std::string&);
  MailboxDefault (const mu_mailbox_t);
  ~MailboxDefault ();
};

}

#endif /* not _MUCPP_MAILBOX_H */

