/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

/* This is an example program to illustrate the use of stream functions.
   It connects to a remote HTTP server and prints the contents of its
   index page */

#include <iostream>
#include <mailutils/cpp/mailutils.h>

using namespace std;
using namespace mailutils;

std::string wbuf = "GET / HTTP/1.0\r\n\r\n";
std::string rbuf;

int
main ()
{
  int off = 0;
  
  try {
    TcpStream stream ("www.gnu.org", 80, MU_STREAM_NONBLOCK);

  connect_again:
    try {
      stream.Open ();
    }
    catch (Stream::EAgain) {
      stream.Wait (MU_STREAM_READY_WR);
      goto connect_again;
    }
    
  write_again:
    try {
      string wbuf (wbuf, off, wbuf.length ());
      stream << wbuf;
    }
    catch (Stream::EAgain) {
      stream.Wait (MU_STREAM_READY_WR);
      off += stream.GetWriten ();
      goto write_again;
    }

    if (stream.GetWriten () != wbuf.length ())
      {
	cerr << "stream.GetWriten() != wbuf length" << endl;
	exit (1);
      }

  read_again:
    do
      {
	try {
	  stream >> rbuf;
	}
	catch (Stream::EAgain) {
	  stream.Wait (MU_STREAM_READY_RD);
	  goto read_again;
	}
	cout << rbuf.substr (0, stream.GetReadn ());
      }
    while (stream.GetReadn ());
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
    exit (1);
  }

  return 0;
}

