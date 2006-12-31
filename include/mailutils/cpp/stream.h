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

#ifndef _MUCPP_STREAM_H
#define _MUCPP_STREAM_H

#include <iostream>
#include <string>
#include <cstdio>
#include <mailutils/stream.h>
#include <mailutils/cpp/error.h>

namespace mailutils
{

class Stream
{
 protected:
  mu_stream_t stm;
  size_t readn;
  size_t writen;
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
  friend class Mailcap;
  friend class Pop3;

 public:
  Stream ();
  Stream (Stream& s);
  Stream (const mu_stream_t);
  ~Stream ();

  void open ();
  void close ();
  void setWaitFlags (int);
  void wait (); // timeval is missing
  void wait (int); // timeval is missing
  void read (char*, size_t, off_t);
  void write (const std::string&, size_t, off_t);
  void readLine (char*, size_t, off_t);
  void sequentialReadLine (char*, size_t);
  void sequentialWrite (const std::string&, size_t);
  void flush ();

  // Inlines
  size_t getReadn () const {
    return readn;
  };
  size_t getWriten () const {
    return writen;
  };

  friend Stream& operator << (Stream&, const std::string&);
  friend Stream& operator >> (Stream&, std::string&);

  // Stream Exceptions
  class EAgain : public Exception {
  public:
    EAgain (const std::string& m, int s) : Exception (m, s) {}
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
  StdioStream (FILE*, int);
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

