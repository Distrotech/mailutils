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

#ifndef _STREAM_H
#define _STREAM_H

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
  stream_t stm;
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
  Stream (const stream_t);
  ~Stream ();

  void Open ();
  void Close ();
  void SetWaitFlags (int);
  void Wait (); // timeval is missing
  void Wait (int); // timeval is missing
  void Read (char*, size_t, off_t);
  void Write (const std::string&, size_t, off_t);
  void ReadLine (char*, size_t, off_t);
  void SequentialReadLine (char*, size_t);
  void SequentialWrite (const std::string&, size_t);
  void Flush ();

  // Inlines
  size_t GetReadn () const {
    return readn;
  };
  size_t GetWriten () const {
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

#endif // not _STREAM_H

