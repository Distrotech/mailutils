/*
   GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2006 Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA
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
	url.parse ();

	cout << "\tscheme <" << url.getScheme () << ">" << endl;
	cout << "\tuser <"   << url.getUser   () << ">" << endl;
	cout << "\tpasswd <" << url.getPasswd () << ">" << endl;
	cout << "\tauth <"   << url.getAuth   () << ">" << endl;
	cout << "\thost <"   << url.getHost   () << ">" << endl;
	cout << "\tport "    << url.getPort   () << endl;
	cout << "\tpath <"   << url.getPath   () << ">" << endl;
	cout << "\tquery <"  << url.getQuery  () << ">" << endl;
      }
      catch (Exception& e) {
	cerr << e.method () << ": " << e.msgError () << endl;
	goto again;
      }
    }
  return 0;
}

