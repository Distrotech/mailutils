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
  int status = 0;
  char* file = "/etc/mailcap";

  try {
    FileStream stream ((std::string) file, MU_STREAM_READ);
    stream.Open ();

    Mailcap mailcap (stream);

    int i;
    size_t count = 0;
    string buffer;

    count = mailcap.GetCount ();
    for (i = 1; i <= count; i++)
      {
	size_t j;
	size_t fields_count = 0;

	cout << "entry[" << i << "]\n";

	MailcapEntry entry = mailcap.GetEntry (i);

	/* typefield.  */
	buffer = entry.GetTypeField ();
	cout << "\ttypefield: " << buffer << endl;

	/* view-command.  */
	buffer = entry.GetViewCommand ();
	cout << "\tview-command: " << buffer << endl;

	/* fields.  */
	fields_count = entry.FieldsCount ();
	for (j = 1; j <= fields_count; j++)
	  {
	    try {
	      buffer = entry.GetField (j);
	    }
	    catch (Exception& e) {
	      cerr << e.Method () << ": cannot retrieve field "
		   << j << ": " << e.MsgError () << endl;
	    }
	    cout << "\tfields[" << j << "]: " << buffer << endl;
	  }

	cout << endl;
      }
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
    exit (1);
  }
  
  return 0;
}
