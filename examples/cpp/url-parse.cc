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

#include <cstdlib>
#include <cstring>

using namespace std;
using namespace mailutils;

int
main ()
{
  char str[1024];

 again:
  while (!cin.getline (str, sizeof (str)).eof ())
    {
      if (strspn (str, " \t") == strlen (str))
        continue; /* skip empty lines */

      try {
	Url url (str);
	url.Parse ();

	cout << "\tscheme <" << url.GetScheme () << ">" << endl;
	cout << "\tuser <"   << url.GetUser   () << ">" << endl;
	cout << "\tpasswd <" << url.GetPasswd () << ">" << endl;
	cout << "\tauth <"   << url.GetAuth   () << ">" << endl;
	cout << "\thost <"   << url.GetHost   () << ">" << endl;
	cout << "\tport "    << url.GetPort   () << endl;
	cout << "\tpath <"   << url.GetPath   () << ">" << endl;
	cout << "\tquery <"  << url.GetQuery  () << ">" << endl;
      }
      catch (Exception& e) {
	cerr << e.Method () << ": " << e.MsgError () << endl;
	goto again;
      }
    }
  return 0;
}

