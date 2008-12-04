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
#include <cstring>
#include <mailutils/cpp/mailutils.h>

#include <mailutils/filter.h>
#include <mailutils/mutil.h>

using namespace std;
using namespace mailutils;

static int
parse (const char *str)
{
  size_t no = 0;
  size_t count = 0;
  std::string buf;

  mu_set_user_email_domain ("localhost");

  try {
    Address address (str);
    count = address.getCount ();
    cout << str << "=> count " << count << endl;

    for (no = 1; no <= count; no++)
      {
	bool isgroup = address.isGroup (no);
	cout << no << " ";
	
	if (isgroup)
	  cout << "group " << address.getPersonal (no) << endl;
	else
	  cout << "email " << address.getEmail (no) << endl;
	
	if (!isgroup)
	  cout << "   personal " << address.getPersonal (no) << endl;
	
	cout << "   comments " << address.getComments (no) << endl;
	cout << "   local-part " << address.getLocalPart (no)
	     << " domain "  << address.getDomain (no) << endl;
	cout << "   route " << address.getRoute (no) << endl;
      }
  }
  catch (Exception& e) {
    cerr << e.method () << ": " << e.msgError () << endl;
  }

  cout << endl;
  return 0;
}

static int
parseinput (void)
{
  char buf[BUFSIZ];

  while (!cin.getline (buf, sizeof (buf)).eof ())
    {
      parse (buf);
    }

  return 0;
}

int
main (int argc, const char *argv[])
{
  argc = 1;

  if (!argv[argc])
    return parseinput ();

  for (; argv[argc]; argc++)
    {
      if (strcmp (argv[argc], "-") == 0)
	parseinput ();
      else
	parse (argv[argc]);
    }

  return 0;
}

