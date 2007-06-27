/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA
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

  mu_register_local_mbox_formats ();

  Message msg;
  Header  hdr;

  try {

    MailboxDefault mbox (argv[1]);

    mbox.open (MU_STREAM_READ);
    total = mbox.messagesCount ();
    cout << "Total: " << total << endl;

    for (int msgno = 1; msgno <= total; msgno++)
    {
      msg = mbox[msgno];
      hdr = msg.getHeader ();
      cout << hdr[MU_HEADER_FROM] << " "
	   << hdr[MU_HEADER_SUBJECT] << endl;
    }

    mbox.close ();
  }
  catch (Exception& e) {
    cerr << e.method () << ": " << e.msgError () << endl;
    exit (1);
  }

  return 0;
}

