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

#include <mailutils/cpp/pop3.h>
#include <mailutils/cpp/iterator.h>
#include <errno.h>

using namespace mailutils;

//
// POP3
//

Pop3 :: Pop3 ()
{
  int status = mu_pop3_create (&pop3);
  if (status)
    throw Exception ("Pop3::Pop3", status);

  this->pStream = 0;
}

Pop3 :: Pop3 (const mu_pop3_t pop3)
{
  if (pop3 == 0)
    throw Exception ("Pop3::Pop3", EINVAL);

  this->pop3 = pop3;
  this->pStream = 0;
}

Pop3 :: ~Pop3 ()
{
  mu_pop3_destroy (&pop3);
}

void
Pop3 :: SetCarrier (const Stream& carrier)
{
  int status = mu_pop3_set_carrier (pop3, carrier.stm);
  if (status)
    throw Exception ("Pop3::SetCarrier", status);

  this->pStream = (Stream*) &carrier;
}

Stream&
Pop3 :: GetCarrier ()
{
  return *pStream;
}

void
Pop3 :: Connect ()
{
  int status = mu_pop3_connect (pop3);
  if (status)
    throw Exception ("Pop3::Connect", status);
}

void
Pop3 :: Disconnect ()
{
  int status = mu_pop3_disconnect (pop3);
  if (status)
    throw Exception ("Pop3::Disconnect", status);
}

void
Pop3 :: SetTimeout (int timeout)
{
  int status = mu_pop3_set_timeout (pop3, timeout);
  if (status)
    throw Exception ("Pop3::SetTimeout", status);
}

int
Pop3 :: GetTimeout ()
{
  int timeout;

  int status = mu_pop3_get_timeout (pop3, &timeout);
  if (status)
    throw Exception ("Pop3::GetTimeout", status);

  return timeout;
}

void
Pop3 :: Stls ()
{
  int status = mu_pop3_stls (pop3);
  if (status)
    throw Exception ("Pop3::Stls", status);
}

Iterator&
Pop3 :: Capa ()
{
  iterator_t mu_itr;

  int status = mu_pop3_capa (pop3, &mu_itr);
  if (status)
    throw Exception ("Pop3::Capa", status);

  return *new Iterator (mu_itr);
}

void
Pop3 :: Dele (unsigned int msgno)
{
  int status = mu_pop3_dele (pop3, msgno);
  if (status)
    throw Exception ("Pop3::Dele", status);
}

size_t
Pop3 :: List (unsigned int msgno)
{
  size_t msg_octet;

  int status = mu_pop3_list (pop3, msgno, &msg_octet);
  if (status)
    throw Exception ("Pop3::List", status);

  return msg_octet;
}

Iterator&
Pop3 :: ListAll ()
{
  iterator_t mu_itr;

  int status = mu_pop3_list_all (pop3, &mu_itr);
  if (status)
    throw Exception ("Pop3::ListAll", status);

  return *new Iterator (mu_itr);
}

void
Pop3 :: Noop ()
{
  int status = mu_pop3_noop (pop3);
  if (status)
    throw Exception ("Pop3::Noop", status);
}

void
Pop3 :: Pass (const char* pass)
{
  int status = mu_pop3_pass (pop3, pass);
  if (status)
    throw Exception ("Pop3::Pass", status);
}

void
Pop3 :: Quit ()
{
  int status = mu_pop3_quit (pop3);
  if (status)
    throw Exception ("Pop3::Quit", status);
}

Stream&
Pop3 :: Retr (unsigned int msgno)
{
  stream_t c_stm;

  int status = mu_pop3_retr (pop3, msgno, &c_stm);
  if (status)
    throw Exception ("Pop3::Retr", status);

  return *new Stream (c_stm);
}

void
Pop3 :: Rset ()
{
  int status = mu_pop3_rset (pop3);
  if (status)
    throw Exception ("Pop3::Rset", status);
}

void
Pop3 :: Stat (unsigned int* count, size_t* octets)
{
  int status = mu_pop3_stat (pop3, count, octets);
  if (status)
    throw Exception ("Pop3::Stat", status);
}

Stream&
Pop3 :: Top (unsigned int msgno, unsigned int lines)
{
  stream_t c_stm;

  int status = mu_pop3_top (pop3, msgno, lines, &c_stm);
  if (status)
    throw Exception ("Pop3::Top", status);

  return *new Stream (c_stm);
}

void
Pop3 :: User (const char* user)
{
  int status = mu_pop3_user (pop3, user);
  if (status)
    throw Exception ("Pop3::User", status);
}

size_t
Pop3 :: ReadLine (char* buf, size_t buflen)
{
  size_t nread;

  int status = mu_pop3_readline (pop3, buf, buflen, &nread);
  if (status)
    throw Exception ("Pop3::ReadLine", status);
}

size_t
Pop3 :: Response (char* buf, size_t buflen)
{
  size_t nread;

  int status = mu_pop3_response (pop3, buf, buflen, &nread);
  if (status)
    throw Exception ("Pop3::Response", status);
}

void
Pop3 :: SendLine (const char* line)
{
  int status = mu_pop3_sendline (pop3, line);
  if (status)
    throw Exception ("Pop3::SendLine", status);
}

void
Pop3 :: Send ()
{
  int status = mu_pop3_send (pop3);
  if (status)
    throw Exception ("Pop3::Send", status);
}

