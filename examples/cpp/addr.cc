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
    count = address.GetCount ();
    cout << str << "=> count " << count << endl;

    for (no = 1; no <= count; no++)
      {
	bool isgroup = address.IsGroup (no);
	cout << no << " ";
	
	if (isgroup)
	  cout << "group " << address.GetPersonal (no) << endl;
	else
	  cout << "email " << address.GetEmail (no) << endl;
	
	if (!isgroup)
	  cout << "   personal " << address.GetPersonal (no) << endl;
	
	cout << "   comments " << address.GetComments (no) << endl;
	cout << "   local-part " << address.GetLocalPart (no)
	     << " domain "  << address.GetDomain (no) << endl;
	cout << "   route " << address.GetRoute (no) << endl;
      }
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
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

