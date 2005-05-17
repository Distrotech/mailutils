/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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
   MA 02110-1301 USA */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/argcv.h>
#include <mailutils/mailutils.h>

void
usage(int code)
{
  printf ("usage: listop [item..]\n");
  exit (code);
}

void
lperror (char *text, int rc)
{
  fprintf (stderr, "%s: %s\n", text, mu_strerror (rc));
  exit (1);
}

void
print (list_t list)
{
  iterator_t itr;
  int rc;
  
  rc = list_get_iterator (list, &itr);
  if (rc)
    lperror ("list_get_iterator", rc);

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *text;

      rc = iterator_current (itr, (void**) &text);
      if (rc)
	lperror ("iterator_current", rc);
      printf ("%s\n", text);
    }
  iterator_destroy (&itr);
}

void
next (iterator_t itr, char *arg)
{
  int skip = arg ? strtoul (arg, NULL, 0) :  1;

  if (skip == 0)
    fprintf (stderr, "next arg?\n");
  while (skip--)
    iterator_next (itr);
}

void
delete (list_t list, int argc, char **argv)
{
  int rc;

  if (argc == 1)
    {
      fprintf (stderr, "del arg?\n");
      return;
    }

  while (--argc)
    {
      rc = list_remove (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "list_remove(%s): %s\n", *argv, mu_strerror (rc));
    }
}

void
add (list_t list, int argc, char **argv)
{
  int rc;
  
  if (argc == 1)
    {
      fprintf (stderr, "add arg?\n");
      return;
    }

  while (--argc)
    {
      rc = list_append (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "list_append: %s\n", mu_strerror (rc));
    }
}

void
prep (list_t list, int argc, char **argv)
{
  int rc;
  
  if (argc == 1)
    {
      fprintf (stderr, "add arg?\n");
      return;
    }

  while (--argc)
    {
      rc = list_prepend (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "list_append: %s\n", mu_strerror (rc));
    }
}

void
repl (list_t list, int argc, char **argv)
{
  int rc;
  
  if (argc != 3)
    {
      fprintf (stderr, "repl src dst?\n");
      return;
    }

  rc = list_replace (list, argv[1], strdup (argv[2]));
  if (rc)
    fprintf (stderr, "list_replace: %s\n", mu_strerror (rc));
}

#define NITR 4

void
iter (int *pnum, int argc, char **argv)
{
  int n;
  
  if (argc != 2)
    {
      fprintf (stderr, "iter num?\n");
      return;
    }

  n = strtoul (argv[1], NULL, 0);
  if (n < 0 || n >= NITR)
    {
      fprintf (stderr, "iter [0-3]?\n");
      return;
    }
  *pnum = n;
}

void
find (iterator_t itr, char *arg)
{
  char *text;
  
  if (!arg)
    {
      fprintf (stderr, "find item?\n");
      return;
    }

  iterator_current (itr, (void**)&text);
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *item;

      iterator_current (itr, (void**)&item);
      if (strcmp (arg, item) == 0)
	return;
    }

  fprintf (stderr, "%s not in list\n", arg);
  
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      char *item;

      iterator_current (itr, (void**)&item);
      if (strcmp (text, item) == 0)
	return;
    }
}

void
help ()
{
  printf ("next [count]\n");
  printf ("first\n");
  printf ("find item\n");
  printf ("del item [item...]\n");
  printf ("add item [item...]\n");
  printf ("prep item [item...]\n");
  printf ("repl old_item new_item\n");
  printf ("print\n");
  printf ("quit\n");
  printf ("iter num\n");
  printf ("help\n");
  printf ("NUMBER\n");
}

void
shell (list_t list)
{
  int num = 0;
  iterator_t itr[NITR];
  int rc;

  for (num = 0; num < NITR; num++)
    {
      rc = list_get_iterator (list, &itr[num]);
      if (rc)
	lperror ("list_get_iterator", rc);
      iterator_first (itr[num]);
    }

  num = 0;
  while (1)
    {
      char *text;
      char buf[80];
      int argc;
      char **argv;
      
      rc = iterator_current (itr[num], (void**) &text);
      if (rc)
	lperror ("iterator_current", rc);

      printf ("%d:(%s)> ", num, text ? text : "NULL");
      if (fgets (buf, sizeof buf, stdin) == NULL)
	return;

      rc = argcv_get (buf, "", "#", &argc, &argv);
      if (rc)
	lperror ("argcv_get", rc);

      if (argc > 0)
	{
	  if (strcmp (argv[0], "next") == 0)
	    next (itr[num], argv[1]);
	  else if (strcmp (argv[0], "first") == 0)
	    iterator_first (itr[num]);
	  else if (strcmp (argv[0], "del") == 0)
	    delete (list, argc, argv);
	  else if (strcmp (argv[0], "add") == 0)
	    add (list, argc, argv);
	  else if (strcmp (argv[0], "prep") == 0)
	    prep (list, argc, argv);
	  else if (strcmp (argv[0], "repl") == 0)
	    repl (list, argc, argv);
	  else if (strcmp (argv[0], "print") == 0)
	    print (list);
	  else if (strcmp (argv[0], "quit") == 0)
	    return;
	  else if (strcmp (argv[0], "iter") == 0)
	    iter (&num, argc, argv);
	  else if (strcmp (argv[0], "find") == 0)
	    find (itr[num], argv[1]);
	  else if (strcmp (argv[0], "help") == 0)
	    help ();
	  else if (argc == 1)
	    {
	      char *p;
	      size_t n = strtoul (argv[0], &p, 0);
	      if (*p != 0)
		fprintf (stderr, "?\n");
	      else
		{
		  rc = list_get (list, n, (void**) &text);
		  if (rc)
		    fprintf (stderr, "list_get: %s\n", mu_strerror (rc));
		  else
		    printf ("%s\n", text);
		}
	    }
	  else
	    fprintf (stderr, "?\n");
	}
      argcv_free (argc, argv);
    }
}

static int
string_comp (const void *item, const void *value)
{
  return strcmp (item, value);
}

int
main (int argc, char **argv)
{
  list_t list;
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

  rc = list_create (&list);
  if (rc)
    lperror ("list_create", rc);
  list_set_comparator (list, string_comp);

  while (argc--)
    {
      rc = list_append (list, *argv++);
      if (rc)
	lperror ("list_append", rc);
    }

  shell (list);
  
  return 0;
}
