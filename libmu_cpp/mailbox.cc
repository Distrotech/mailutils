/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006, 2007, 2009 Free Software Foundation, Inc.

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

#include <mailutils/cpp/mailbox.h>
#include <mailutils/cpp/error.h>
#include <errno.h>

using namespace mailutils;

//
// MailboxBase
//

void
MailboxBase :: open ()
{
  int status = mu_mailbox_open (mbox, MU_STREAM_READ);
  if (status)
    throw Exception ("MailboxBase::open", status);
}

void
MailboxBase :: open (int flag)
{
  int status = mu_mailbox_open (mbox, flag);
  if (status)
    throw Exception ("MailboxBase::open", status);
}

void
MailboxBase :: close ()
{
  int status = mu_mailbox_close (mbox);
  if (status)
    throw Exception ("MailboxBase::close", status);
}

Debug&
MailboxBase :: get_debug ()
{
  mu_debug_t c_dbg;

  int status = mu_mailbox_get_debug (mbox, &c_dbg);
  if (status)
    throw Exception ("MailboxBase::get_debug", status);

  return *new Debug (c_dbg);
}

size_t
MailboxBase :: messages_count ()
{
  size_t total;
  mu_mailbox_messages_count (mbox, &total);
  return total;
}

Message&
MailboxBase :: get_message (size_t num)
{
  mu_message_t c_msg;

  int status = mu_mailbox_get_message (mbox, num, &c_msg);
  if (status)
    throw Exception ("MailboxBase::get_message", status);

  return *new Message (c_msg);
}

//
// Mailbox
//

Mailbox :: Mailbox (const std::string& name)
{
  int status = mu_mailbox_create (&mbox, name.c_str ());
  if (status)
    throw Exception ("Mailbox::Mailbox", status);
}

Mailbox :: Mailbox (const mu_mailbox_t mbox)
{
  if (mbox == 0)
    throw Exception ("Mailbox::Mailbox", EINVAL);

  this->mbox = mbox;
}

Mailbox :: ~Mailbox ()
{
  mu_mailbox_destroy (&mbox);
}

//
// MailboxDefault
//

MailboxDefault :: MailboxDefault (const std::string& name)
{
  int status = mu_mailbox_create_default (&mbox, name.c_str ());
  if (status)
    throw Exception ("MailboxDefault::MailboxDefault", status);
}

MailboxDefault :: MailboxDefault (const mu_mailbox_t mbox)
{
  if (mbox == 0)
    throw Exception ("MailboxDefault::MailboxDefault", EINVAL);

  this->mbox = mbox;
}

MailboxDefault :: ~MailboxDefault ()
{
  mu_mailbox_destroy (&mbox);
}

