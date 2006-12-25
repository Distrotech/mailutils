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

#include <mailutils/cpp/mailer.h>
#include <errno.h>

using namespace mailutils;

//
// Mailer
//

Mailer :: Mailer (const std::string& url)
{
  int status = mu_mailer_create (&mailer, url.c_str ());
  if (status)
    throw Exception ("Mailer::Mailer", status);
}

Mailer :: Mailer (const mu_mailer_t mailer)
{
  if (mailer == 0)
    throw Exception ("Mailer::Mailer", EINVAL);

  this->mailer = mailer;
}

Mailer :: ~Mailer ()
{
  mu_mailer_destroy (&mailer);
}

void
Mailer :: open (int flags)
{
  int status = mu_mailer_open (mailer, flags);
  if (status)
    throw Exception ("Mailer::open", status);
}

void
Mailer :: close ()
{
  int status = mu_mailer_close (mailer);
  if (status)
    throw Exception ("Mailer::close", status);
}

void
Mailer :: sendMessage (const Message& msg, const Address& from,
		       const Address& to)
{
  int status = mu_mailer_send_message (mailer, msg.msg,
				       from.addr, to.addr);
  if (status)
    throw Exception ("Mailer::sendMessage", status);
}

