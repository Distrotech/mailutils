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

#include <mailutils/cpp/mailbox.h>
#include <mailutils/cpp/message.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// MailboxBase
//

void
MailboxBase :: Open (int flag)
{
  int status = mailbox_open (mbox, flag);
  if (status)
    throw Exception ("MailboxBase::Open", status);
}

void
MailboxBase :: Close ()
{
  int status = mailbox_close (mbox);
  if (status)
    throw Exception ("MailboxBase::Close", status);
}

size_t
MailboxBase :: MessagesCount ()
{
  size_t total;
  mailbox_messages_count (mbox, &total);
  return total;
}

Message&
MailboxBase :: GetMessage (size_t num)
{
  message_t c_msg;

  int status = mailbox_get_message (mbox, num, &c_msg);
  if (status)
    throw Exception ("MailboxBase::GetMessage", status);

  return *new Message (c_msg);
}

Message&
MailboxBase :: operator [] (size_t num)
{
  return this->GetMessage (num);
}

//
// Mailbox
//

Mailbox :: Mailbox (const std::string& name)
{
  int status = mailbox_create (&mbox, name.c_str ());
  if (status)
    throw Exception ("Mailbox::Mailbox", status);
}

Mailbox :: Mailbox (const mailbox_t mbox)
{
  if (mbox == 0)
    throw Exception ("Mailbox::Mailbox", EINVAL);

  this->mbox = mbox;
}

Mailbox :: ~Mailbox ()
{
  mailbox_destroy (&mbox);
}

//
// MailboxDefault
//

MailboxDefault :: MailboxDefault (const std::string& name)
{
  int status = mailbox_create_default (&mbox, name.c_str ());
  if (status)
    throw Exception ("MailboxDefault::MailboxDefault", status);
}

MailboxDefault :: MailboxDefault (const mailbox_t mbox)
{
  if (mbox == 0)
    throw Exception ("MailboxDefault::MailboxDefault", EINVAL);

  this->mbox = mbox;
}

MailboxDefault :: ~MailboxDefault ()
{
  mailbox_destroy (&mbox);
}

