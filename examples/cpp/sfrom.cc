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

#include <mailutils/registrar.h> // tmp
#include <mailutils/list.h> // tmp

using namespace std;
using namespace mailutils;

int main (int argc, char* argv[])
{
  size_t total = 0;

  if (argc == 1)
    exit (0);

  mu_register_local_mbox_formats();

  Message msg;
  Header  hdr;

  try {

    MailboxDefault mbox (argv[1]);

    mbox.Open (MU_STREAM_READ);
    total = mbox.MessagesCount ();
    cout << "Total: " << total << endl;

    for (int msgno = 1; msgno <= total; msgno++)
    {
      msg = mbox[msgno];
      hdr = msg.GetHeader ();
      cout << hdr[MU_HEADER_FROM] << " "
	   << hdr[MU_HEADER_SUBJECT] << endl;
    }

    mbox.Close ();
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
    exit (1);
  }

  return 0;
}

