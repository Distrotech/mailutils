/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

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

#include <mailutils/cpp/sieve.h>

using namespace mailutils;

//
// SieveMachine
//

SieveMachine :: SieveMachine ()
{
  int status = mu_sieve_machine_init (&mach);
  if (status)
    throw Exception ("SieveMachine::SieveMachine", status);
}

SieveMachine :: SieveMachine (const mu_sieve_machine_t mach)
{
  if (mach == 0)
    throw Exception ("SieveMachine::SieveMachine", EINVAL);

  this->mach = mach;
}

SieveMachine :: ~SieveMachine ()
{
  if (mach)
    mu_sieve_machine_destroy (&mach);
}

SieveMachine&
SieveMachine :: operator = (const SieveMachine& m)
{
  if (this != &m)
    {
      if (this->mach)
	mu_sieve_machine_destroy (&this->mach);
      mu_sieve_machine_dup (m.mach, &this->mach);
    }
  return *this;
}

void
SieveMachine :: compile (const std::string& name)
{
  int status = mu_sieve_compile (mach, name.c_str ());
  if (status)
    throw Exception ("SieveMachine::compile", status);
}

void
SieveMachine :: disass ()
{
  int status = mu_sieve_disass (mach);
  if (status)
    throw Exception ("SieveMachine::disass", status);
}

void
SieveMachine :: mailbox (const Mailbox& mbox)
{
  int status = mu_sieve_mailbox (mach, mbox.mbox);
  if (status)
    throw Exception ("SieveMachine::mailbox", status);
}

void
SieveMachine :: message (const Message& msg)
{
  int status = mu_sieve_message (mach, msg.msg);
  if (status)
    throw Exception ("SieveMachine::message", status);
}


