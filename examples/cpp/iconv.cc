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

#include <iostream>
#include <mailutils/cpp/mailutils.h>

using namespace std;
using namespace mailutils;

int
main (int argc, char **argv)
{
  size_t total = 0;
  char buffer[80];

  if (argc != 3)
    {
      cerr << "usage: " << argv[0] << " from-code to-code" << endl;
      return 1;
    }

  try {
    StdioStream *in = new StdioStream(stdin, 0);
    in->Open ();

    FilterStream cvt;
    cvt.IconvCreate (*in, (string)argv[1], (string)argv[2], 0, mu_fallback_none);
    cvt.Open ();
    delete in;
    
    StdioStream out (stdout, 0);
    out.Open ();

    do {
      cvt.Read (buffer, sizeof (buffer), total);
      out.SequentialWrite (buffer, cvt.GetReadn ());
      total += cvt.GetReadn ();
    } while (cvt.GetReadn ());

    out.Flush ();
    delete in;
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
    exit (1);
  }

  return 0;
}
