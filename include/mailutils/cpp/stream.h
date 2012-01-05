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

#ifndef _MUCPP_STREAM_H
#define _MUCPP_STREAM_H

#include <string>
#include <errno.h>
#include <mailutils/stream.h>
#include <mailutils/prog.h>
#include <mailutils/wordsplit.h>
#include <mailutils/cpp/error.h>

namespace mailutils
{

class Stream
{
 protected:
  mu_stream_t stm;
  size_t read_count;
  size_t write_count;
  int wflags;
  bool opened;
  size_t reference_count;

  // Inlines
  void reference () {
    reference_count++;
  }
  bool dereference () {
    return --reference_count == 0;
  }

  // Friends
  friend class FilterStream;
  friend class FilterProgStream;
  friend class FilterIconvStream;
  friend class Folder;
  friend class Mailcap;
  friend class Message;
  friend class Pop3;

 public:
  Stream ();
  Stream (Stream& s);
  Stream (const mu_stream_t);
  ~Stream ();

  void open ();
  void close ();
  void set_waitflags (int flags);
  void wait (); // timeval is missing
  void wait (int flags); // timeval is missing
  int  read (void* rbuf, size_t size);
  int  write (void* wbuf, size_t size);
  int  write (const std::string& wbuf, size_t size);
  int  readline (char* rbuf, size_t size);
  void flush ();

  // Inlines
  size_t get_read_count () const {
    return read_count;
  };
  size_t get_write_count () const {
    return write_count;
  };

  friend Stream& operator << (Stream&, const std::string&);
  friend Stream& operator >> (Stream&, std::string&);

  // Stream Exceptions
  class EAgain : public Exception {
  public:
    EAgain (const char* m, int s) : Exception (m, s) {}
  };
};

class TcpStream : public Stream
{
 public:
  TcpStream (const std::string&, int, int);
};

class FileStream : public Stream
{
 public:
  FileStream (const std::string&, int);
};

class StdioStream : public Stream
{
 public:
  StdioStream (int fd, int flags);
};

class ProgStream : public Stream
{
 public:
  ProgStream (const std::string&, int);
};

class FilterProgStream : public Stream
{
 private:
  Stream *input;
 public:
  FilterProgStream (const std::string&, Stream&);
  FilterProgStream (const std::string&, Stream*);
};

}

#endif // not _MUCPP_STREAM_H

