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

#include <mailutils/cpp/stream.h>
#include <errno.h>

using namespace mailutils;

//
// Stream
//

Stream :: Stream ()
{
  this->stm = 0;
  this->opened = false;
  reference ();
}

Stream :: Stream (const stream_t stm)
{
  if (stm == 0)
    throw Exception ("Stream::Stream", EINVAL);

  this->stm = stm;
  this->opened = false;
  reference ();
}

Stream :: Stream (Stream& s)
{
  s.reference ();
  this->stm = s.stm;
}

Stream :: ~Stream ()
{
  if (dereference ())
    {
      Close ();
      if (this->stm)
	stream_destroy (&stm, NULL);
    }
}

void
Stream :: Open ()
{
  int status = stream_open (stm);
  if (status == EAGAIN)
    throw Stream::EAgain ("Stream::Open", status);
  else if (status)
    throw Exception ("Stream::Open", status);

  this->opened = true;
}

void
Stream :: Close ()
{
  if (this->opened)
    {
      int status = stream_close (stm);
      if (status)
	throw Exception ("Stream::Close", status);

      this->opened = false;
    }
}

void
Stream :: SetWaitFlags (int flags)
{
  this->wflags = flags;
}

void
Stream :: Wait ()
{
  int status = stream_wait (stm, &wflags, NULL);
  if (status)
    throw Exception ("Stream::Wait", status);
}

void
Stream :: Wait (int flags)
{
  this->wflags = flags;
  int status = stream_wait (stm, &wflags, NULL);
  if (status)
    throw Exception ("Stream::Wait", status);
}

void
Stream :: Read (char* rbuf, size_t size, off_t offset)
{
  int status = stream_read (stm, rbuf, size, offset, &readn);
  if (status == EAGAIN)
    throw Stream::EAgain ("Stream::Read", status);
  else if (status)
    throw Exception ("Stream::Read", status);
}

void
Stream :: Write (const std::string& wbuf, size_t size, off_t offset)
{
  int status = stream_write (stm, wbuf.c_str (), size, offset, &writen);
  if (status == EAGAIN)
    throw Stream::EAgain ("Stream::Write", status);
  else if (status)
    throw Exception ("Stream::Write", status);
}

void
Stream :: ReadLine (char* rbuf, size_t size, off_t offset)
{
  int status = stream_readline (stm, rbuf, size, offset, &readn);
  if (status == EAGAIN)
    throw Stream::EAgain ("Stream::ReadLine", status);
  else if (status)
    throw Exception ("Stream::ReadLine", status);
}

void
Stream :: SequentialReadLine (char* rbuf, size_t size)
{
  int status = stream_sequential_readline (stm, rbuf, size, &readn);
  if (status)
    throw Exception ("Stream::SequentialReadLine", status);
}

void
Stream :: SequentialWrite (const std::string& wbuf, size_t size)
{
  int status = stream_sequential_write (stm, wbuf.c_str (), size);
  if (status)
    throw Exception ("Stream::SequentialWrite", status);
}

void
Stream :: Flush ()
{
  int status = stream_flush (stm);
  if (status)
    throw Exception ("Stream::Flush", status);
}

namespace mailutils
{
  Stream&
  operator << (Stream& stm, const std::string& wbuf)
  {
    stm.Write (wbuf, wbuf.length (), 0);
    return stm;
  }

  Stream&
  operator >> (Stream& stm, std::string& rbuf)
  {
    char tmp[1024];
    stm.Read (tmp, sizeof (tmp), 0);
    rbuf = std::string (tmp);
    return stm;
  }
}

//
// TcpStream
//

TcpStream :: TcpStream (const std::string& host, int port, int flags)
{
  int status = tcp_stream_create (&stm, host.c_str (), port, flags);
  if (status)
    throw Exception ("TcpStream::TcpStream", status);
}

//
// FileStream
//

FileStream :: FileStream (const std::string& filename, int flags)
{
  int status = file_stream_create (&stm, filename.c_str (), flags);
  if (status)
    throw Exception ("FileStream::FileStream", status);
}

//
// StdioStream
//

StdioStream :: StdioStream (FILE* fp, int flags)
{
  int status = stdio_stream_create (&stm, fp, flags);
  if (status)
    throw Exception ("StdioStream::StdioStream", status);
}

//
// ProgStream
//

ProgStream :: ProgStream (const std::string& progname, int flags)
{
  int status = prog_stream_create (&stm, progname.c_str (), flags);
  if (status)
    throw Exception ("ProgStream::ProgStream", status);
}

//
// FilterProgStream
//

FilterProgStream :: FilterProgStream (const std::string& progname,
				      Stream& input)
{
  int status = filter_prog_stream_create (&stm, progname.c_str (),
					  input.stm);
  this->input = new Stream (input);
  if (status)
    throw Exception ("FilterProgStream::FilterProgStream", status);
}

FilterProgStream :: FilterProgStream (const std::string& progname,
				      Stream* input)
{
  int status = filter_prog_stream_create (&stm, progname.c_str (),
					  input->stm);
  this->input = new Stream (*input);
  if (status)
    throw Exception ("FilterProgStream::FilterProgStream", status);
}

