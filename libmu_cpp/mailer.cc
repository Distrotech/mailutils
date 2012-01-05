/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006-2007, 2009-2012 Free Software Foundation,
   Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#include <mailutils/cpp/mailer.h>

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
Mailer :: open ()
{
  int status = mu_mailer_open (mailer, 0);
  if (status)
    throw Exception ("Mailer::open", status);
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
Mailer :: send_message (const Message& msg, const Address& from,
			const Address& to)
{
  int status = mu_mailer_send_message (mailer, msg.msg,
				       from.addr, to.addr);
  if (status)
    throw Exception ("Mailer::send_message", status);
}

