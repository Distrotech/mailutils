/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005, 2007 Free Software Foundation, Inc.

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
print (mu_list_t list)
{
  mu_iterator_t itr;
  int rc;
  
  rc = mu_list_get_iterator (list, &itr);
  if (rc)
    lperror ("mu_list_get_iterator", rc);

  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *text;

      rc = mu_iterator_current (itr, (void**) &text);
      if (rc)
	lperror ("mu_iterator_current", rc);
      printf ("%s\n", text);
    }
  mu_iterator_destroy (&itr);
}

void
next (mu_iterator_t itr, char *arg)
{
  int skip = arg ? strtoul (arg, NULL, 0) :  1;

  if (skip == 0)
    fprintf (stderr, "next arg?\n");
  while (skip--)
    mu_iterator_next (itr);
}

void
delete (mu_list_t list, int argc, char **argv)
{
  int rc;

  if (argc == 1)
    {
      fprintf (stderr, "del arg?\n");
      return;
    }

  while (--argc)
    {
      rc = mu_list_remove (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "mu_list_remove(%s): %s\n", *argv, mu_strerror (rc));
    }
}

void
add (mu_list_t list, int argc, char **argv)
{
  int rc;
  
  if (argc == 1)
    {
      fprintf (stderr, "add arg?\n");
      return;
    }

  while (--argc)
    {
      rc = mu_list_append (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "mu_list_append: %s\n", mu_strerror (rc));
    }
}

void
prep (mu_list_t list, int argc, char **argv)
{
  int rc;
  
  if (argc == 1)
    {
      fprintf (stderr, "add arg?\n");
      return;
    }

  while (--argc)
    {
      rc = mu_list_prepend (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "mu_list_append: %s\n", mu_strerror (rc));
    }
}

void
ins (mu_list_t list, int argc, char **argv)
{
  int rc;
  char *item;
  char *new_item;
  
  if (argc < 3 || argc > 4)
    {
      fprintf (stderr, "ins [before] item new_item?\n");
      return;
    }

  if (argc == 4)
    {
      if (strcmp (argv[1], "before"))
	{
	  fprintf (stderr, "ins before item new_item?\n");
	  return;
	}
	
      item = argv[2];
      new_item = argv[3];
    }
  else
    {
      item = argv[1];
      new_item = argv[2];
    }
  
  rc = mu_list_insert (list, item, strdup (new_item), argc == 4);
  if (rc)
    fprintf (stderr, "mu_list_insert: %s\n", mu_strerror (rc));
}


void
repl (mu_list_t list, int argc, char **argv)
{
  int rc;
  
  if (argc != 3)
    {
      fprintf (stderr, "repl src dst?\n");
      return;
    }

  rc = mu_list_replace (list, argv[1], strdup (argv[2]));
  if (rc)
    fprintf (stderr, "mu_list_replace: %s\n", mu_strerror (rc));
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
find (mu_iterator_t itr, char *arg)
{
  char *text;
  
  if (!arg)
    {
      fprintf (stderr, "find item?\n");
      return;
    }

  mu_iterator_current (itr, (void**)&text);
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *item;

      mu_iterator_current (itr, (void**)&item);
      if (strcmp (arg, item) == 0)
	return;
    }

  fprintf (stderr, "%s not in list\n", arg);
  
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      char *item;

      mu_iterator_current (itr, (void**)&item);
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
  printf ("ins [before] item new_item\n");
  printf ("print\n");
  printf ("quit\n");
  printf ("iter num\n");
  printf ("help\n");
  printf ("NUMBER\n");
}

void
shell (mu_list_t list)
{
  int num = 0;
  mu_iterator_t itr[NITR];
  int rc;

  for (num = 0; num < NITR; num++)
    {
      rc = mu_list_get_iterator (list, &itr[num]);
      if (rc)
	lperror ("mu_list_get_iterator", rc);
      mu_iterator_first (itr[num]);
    }

  num = 0;
  while (1)
    {
      char *text;
      char buf[80];
      int argc;
      char **argv;
      
      rc = mu_iterator_current (itr[num], (void**) &text);
      if (rc)
	lperror ("mu_iterator_current", rc);

      printf ("%d:(%s)> ", num, text ? text : "NULL");
      if (fgets (buf, sizeof buf, stdin) == NULL)
	return;

      rc = mu_argcv_get (buf, "", "#", &argc, &argv);
      if (rc)
	lperror ("mu_argcv_get", rc);

      if (argc > 0)
	{
	  if (strcmp (argv[0], "next") == 0)
	    next (itr[num], argv[1]);
	  else if (strcmp (argv[0], "first") == 0)
	    mu_iterator_first (itr[num]);
	  else if (strcmp (argv[0], "del") == 0)
	    delete (list, argc, argv);
	  else if (strcmp (argv[0], "add") == 0)
	    add (list, argc, argv);
	  else if (strcmp (argv[0], "prep") == 0)
	    prep (list, argc, argv);
	  else if (strcmp (argv[0], "ins") == 0)
	    ins (list, argc, argv);
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
		  rc = mu_list_get (list, n, (void**) &text);
		  if (rc)
		    fprintf (stderr, "mu_list_get: %s\n", mu_strerror (rc));
		  else
		    printf ("%s\n", text);
		}
	    }
	  else
	    fprintf (stderr, "?\n");
	}
      mu_argcv_free (argc, argv);
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
  mu_list_t list;
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

  rc = mu_list_create (&list);
  if (rc)
    lperror ("mu_list_create", rc);
  mu_list_set_comparator (list, string_comp);

  while (argc--)
    {
      rc = mu_list_append (list, *argv++);
      if (rc)
	lperror ("mu_list_append", rc);
    }

  shell (list);
  
  return 0;
}
