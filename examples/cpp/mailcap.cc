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

using namespace std;
using namespace mailutils;

int
main (int argc, char **argv)
{
  int status = 0;
  char* file = "/etc/mailcap";

  try {
    FileStream stream ((std::string) file, MU_STREAM_READ);
    stream.open ();

    Mailcap mailcap (stream);

    int i;
    size_t count = 0;
    string buffer;

    count = mailcap.getCount ();
    for (i = 1; i <= count; i++)
      {
	size_t j;
	size_t fields_count = 0;

	cout << "entry[" << i << "]\n";

	MailcapEntry entry = mailcap.getEntry (i);

	/* typefield.  */
	buffer = entry.getTypeField ();
	cout << "\ttypefield: " << buffer << endl;

	/* view-command.  */
	buffer = entry.getViewCommand ();
	cout << "\tview-command: " << buffer << endl;

	/* fields.  */
	fields_count = entry.fieldsCount ();
	for (j = 1; j <= fields_count; j++)
	  {
	    try {
	      buffer = entry.getField (j);
	    }
	    catch (Exception& e) {
	      cerr << e.method () << ": cannot retrieve field "
		   << j << ": " << e.msgError () << endl;
	    }
	    cout << "\tfields[" << j << "]: " << buffer << endl;
	  }

	cout << endl;
      }
  }
  catch (Exception& e) {
    cerr << e.method () << ": " << e.msgError () << endl;
    exit (1);
  }
  
  return 0;
}
