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

#ifndef _POP3_H
#define _POP3_H

#include <iostream>
#include <mailutils/pop3.h>
#include <mailutils/cpp/list.h>
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

  void SetCarrier (const Stream&);
  Stream& GetCarrier ();
  void Connect ();
  void Disconnect ();
  void SetTimeout (int);
  int GetTimeout ();

  void Stls ();
  Iterator& Capa ();
  void Dele (unsigned int);
  size_t List (unsigned int);
  Iterator& ListAll ();
  void Noop ();
  void Pass (const char*);
  void Quit ();
  Stream& Retr (unsigned int);
  void Rset ();
  void Stat (unsigned int*, size_t*);
  Stream& Top (unsigned int, unsigned int);
  void User (const char*);

  size_t ReadLine (char*, size_t);
  size_t Response (char*, size_t);
  void SendLine (const char*);
  void Send ();
};

}

#endif // not _POP3_H

