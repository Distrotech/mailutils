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

#ifndef _MUCPP_MESSAGE_H
#define _MUCPP_MESSAGE_H

#include <mailutils/message.h>
#include <mailutils/cpp/header.h>
#include <mailutils/cpp/body.h>
#include <mailutils/cpp/stream.h>

namespace mailutils
{

class Message
{
 protected:
  mu_message_t msg;
  bool owner;

  friend class Mailer;
  friend class Mime;

 public:
  Message ();
  Message (const mu_message_t);
  Message& operator = (const Message&);
  ~Message ();

  Header& get_header ();
  Body& get_body ();
  Stream& get_stream ();
  void set_stream (const Stream& stream);

  bool is_multipart ();
  size_t size ();
  size_t lines ();
  size_t get_num_parts ();
  Message& get_part (const size_t npart);

  void save_attachment ();
  void save_attachment (const std::string& filename);
  Message& unencapsulate ();
  std::string get_attachment_name ();
};

}

#endif // not _MUCPP_MESSAGE_H

