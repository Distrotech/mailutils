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
#include <cstdlib>
#include <mailutils/cpp/mailutils.h>
#include <mailutils/argcv.h>

using namespace std;
using namespace mailutils;

void
usage (int code)
{
  cout << "usage: listop [item..]" << endl;
  exit (code);
}

void
print (List& lst)
{
  Iterator itr (lst);

  for (itr.First (); !itr.IsDone (); itr++)
    {
      char* text = (char *) itr.Current ();
      cout << text << endl;
    }
}

void
next (Iterator* itr, char *arg)
{
  int skip = arg ? strtoul (arg, NULL, 0) :  1;

  if (skip == 0)
    cout << "next arg?" << endl;
  while (skip--)
    itr->Next ();
}

void
del (List& lst, int argc, char **argv)
{
  int rc;
  if (argc == 1)
    {
      cerr << "del arg?" << endl;
      return;
    }

  while (--argc)
    {
      try {
	lst.Remove (strdup (*++argv));
      }
      catch (Exception& e) {
	cerr << e.Method () << ": " << e.MsgError () << endl;
      }
    }
}

void
add (List& lst, int argc, char **argv)
{
  int rc;
  
  if (argc == 1)
    {
      cerr << "add arg?" << endl;
      return;
    }

  while (--argc)
    {
      try {
	lst.Append (strdup (*++argv));
      }
      catch (Exception& e) {
	cerr << e.Method () << ": " << e.MsgError () << endl;
      }
    }
}

void
prep (List& lst, int argc, char **argv)
{
  int rc;
  if (argc == 1)
    {
      cerr << "add arg?" << endl;
      return;
    }

  while (--argc)
    {
      try {
	lst.Prepend (strdup (*++argv));
      }
      catch (Exception& e) {
	cerr << e.Method () << ": " << e.MsgError () << endl;
      }
    }
}

void
repl (List& lst, int argc, char **argv)
{
  int rc;
  if (argc != 3)
    {
      cerr << "repl src dst?" << endl;
      return;
    }

  try {
    lst.Replace (argv[1], strdup (argv[2]));
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
  }
}

#define NITR 4

void
iter (int *pnum, int argc, char** argv)
{
  int n;
  if (argc != 2)
    {
      cerr << "iter num?" << endl;
      return;
    }

  n = strtoul (argv[1], NULL, 0);
  if (n < 0 || n >= NITR)
    {
      cerr << "iter [0-3]?" << endl;
      return;
    }
  *pnum = n;
}

void
find (Iterator* itr, char* arg)
{
  char *text;
  if (!arg)
    {
      cerr << "find item?" << endl;
      return;
    }

  itr->Current ((void**) &text);
  for (itr->First (); !itr->IsDone (); itr->Next ())
    {
      char *item;

      itr->Current ((void**) &item);
      if (!strcmp (arg, item))
	return;
    }

  cerr << arg << " not in list" << endl;

  for (itr->First (); !itr->IsDone (); itr->Next ())
    {
      char *item;

      itr->Current ((void**) &item);
      if (!strcmp (text, item))
	return;
    }
}

void
help ()
{
  cout << "next [count]\n";
  cout << "first\n";
  cout << "find item\n";
  cout << "del item [item...]\n";
  cout << "add item [item...]\n";
  cout << "prep item [item...]\n";
  cout << "repl old_item new_item\n";
  cout << "print\n";
  cout << "quit\n";
  cout << "iter num\n";
  cout << "help\n";
  cout << "NUMBER\n";
}

void
shell (List& lst)
{
  int rc;
  int num = 0;
  Iterator* itr[NITR];

  for (num = 0; num < NITR; num++)
    {
      itr[num] = new Iterator (lst);
      itr[num]->First ();
    }

  num = 0;
  while (1)
    {
      char *text;
      char buf[80];
      int argc;
      char **argv;
      
      try {
	itr[num]->Current ((void**) &text);
      }
      catch (Exception& e) {
	cerr << e.Method () << ": " << e.MsgError () << endl;
      }

      cout << num << ":(" << (text ? text : "NULL") << ")> ";
      if (cin.getline (buf, sizeof (buf)).eof ())
	return;

      rc = argcv_get (buf, "", "#", &argc, &argv);
      if (rc)
	cerr << "argcv_get: " << rc << endl;

      if (argc > 0)
	{
	  if (!strcmp (argv[0], "next"))
	    next (itr[num], argv[1]);
	  else if (!strcmp (argv[0], "first"))
	    itr[num]->First ();
	  else if (!strcmp (argv[0], "del"))
	    del (lst, argc, argv);
	  else if (!strcmp (argv[0], "add"))
	    add (lst, argc, argv);
	  else if (!strcmp (argv[0], "prep"))
	    prep (lst, argc, argv);
	  else if (!strcmp (argv[0], "repl"))
	    repl (lst, argc, argv);
	  else if (!strcmp (argv[0], "print"))
	    print (lst);
	  else if (!strcmp (argv[0], "quit"))
	    return;
	  else if (!strcmp (argv[0], "iter"))
	    iter (&num, argc, argv);
	  else if (!strcmp (argv[0], "find"))
	    find (itr[num], argv[1]);
	  else if (!strcmp (argv[0], "help"))
	    help ();
	  else if (argc == 1)
	    {
	      char* p;
	      size_t n = strtoul (argv[0], &p, 0);
	      if (*p != 0)
		cerr << "?" << endl;
	      else
		{
		  try {
		    text = (char*) lst[n];
		  }
		  catch (Exception& e) {
		    cerr << e.Method () << ": " << e.MsgError () << endl;
		  }

		  // else
		  cout << text << endl;
		}
	    }
	  else
	    cerr << "?" << endl;
	}
      argcv_free (argc, argv);
    }
}

static int
string_comp (const void* item, const void* value)
{
  return strcmp ((const char*) item, (const char*) value);
}

int
main (int argc, char **argv)
{
  int rc;

  while ((rc = getopt (argc, argv, "h")) != EOF)
    switch (rc)
      {
      case 'h':
	usage (0);
	
      default:
	usage (1);
      }

  argc -= optind;
  argv += optind;

  try {
    List lst;
    lst.SetComparator (string_comp);
    
    while (argc--)
      {
	lst.Append (*argv++);
      }

    shell (lst);
  }
  catch (Exception& e) {
    cerr << e.Method () << ": " << e.MsgError () << endl;
  }

  return 0;
}

