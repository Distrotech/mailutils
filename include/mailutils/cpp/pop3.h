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

#ifndef _MUCPP_POP3_H
#define _MUCPP_POP3_H

#include <iostream>
#include <mailutils/pop3.h>
#include <mailutils/cpp/list.h>
#include <mailutils/cpp/iterator.h>
#include <mailutils/cpp/stream.h>

namespace mailutils
{

class Pop3
{
 protected:
  mu_pop3_t pop3;
  Stream* pStream;

 public:
  Pop3 ();
  Pop3 (const mu_pop3_t);
  ~Pop3 ();

  void setCarrier (const Stream&);
  Stream& getCarrier ();
  void connect ();
  void disconnect ();
  void setTimeout (int);
  int getTimeout ();

  void stls ();
  Iterator& capa ();
  void dele (unsigned int);
  size_t list (unsigned int);
  Iterator& listAll ();
  void noop ();
  void pass (const char*);
  void quit ();
  Stream& retr (unsigned int);
  void rset ();
  void stat (unsigned int*, size_t*);
  Stream& top (unsigned int, unsigned int);
  void user (const char*);

  size_t readLine (char*, size_t);
  size_t response (char*, size_t);
  void sendLine (const char*);
  void send ();
};

}

#endif // not _MUCPP_POP3_H

