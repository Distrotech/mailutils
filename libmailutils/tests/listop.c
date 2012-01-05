/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/mailutils.h>

static int interactive;

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
  size_t count;
  int rc;
  
  rc = mu_list_get_iterator (list, &itr);
  if (rc)
    lperror ("mu_list_get_iterator", rc);

  rc = mu_list_count (list, &count);
  if (rc)
    lperror ("mu_iterator_current", rc);

  printf ("# items: %lu\n", (unsigned long) count);
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
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
count (mu_list_t list)
{
  size_t n;
  int rc;

  rc = mu_list_count (list, &n);
  if (rc)
    lperror ("mu_iterator_current", rc);
  else
    printf ("%lu\n", (unsigned long) n);
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
      rc = mu_list_remove (list, *++argv);
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

static mu_list_t
read_list (int argc, char **argv)
{
  int rc;
  mu_list_t list;
  
  rc = mu_list_create (&list);
  if (rc)
    {
      fprintf (stderr, "creating temp list: %s\n", mu_strerror (rc));
      return NULL;
    }
  mu_list_set_destroy_item (list, mu_list_free_item);
  for (; argc; argc--, argv++)
    {
      rc = mu_list_append (list, strdup (*argv));
      if (rc)
	{
	  mu_list_destroy (&list);
	  fprintf (stderr, "adding to temp list: %s\n", mu_strerror (rc));
	  break;
	}
    }
  return list;
}

void
ins (mu_list_t list, int argc, char **argv)
{
  int an;
  int rc;
  char *item;
  int insert_before = 0;
  
  if (argc < 3)
    {
      fprintf (stderr, "ins [before] item new_item [new_item*]?\n");
      return;
    }

  an = 1;
  if (strcmp (argv[1], "before") == 0)
    {
      an++;
      insert_before = 1;
    }
  else if (strcmp (argv[1], "after") == 0)
    {
      an++;
      insert_before = 0;
    }

  item = argv[an++];
  
  if (an + 1 == argc)
    rc = mu_list_insert (list, item, strdup (argv[an]), insert_before);
  else
    {
      mu_list_t tmp = read_list (argc - an, argv + an);
      if (!tmp)
	return;
      rc = mu_list_insert_list (list, item, tmp, insert_before);
      mu_list_destroy (&tmp);
    }

  if (rc)
    lperror ("mu_list_insert", rc);
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

void
ictl_tell (mu_iterator_t itr, int argc)
{
  size_t pos;
  int rc;

  if (argc)
    {
      fprintf (stderr, "ictl tell?\n");
      return;
    }
  
  rc = mu_iterator_ctl (itr, mu_itrctl_tell, &pos);
  if (rc)
    lperror ("mu_iterator_ctl", rc);
  printf ("%lu\n", (unsigned long) pos);
}

void
ictl_del (mu_iterator_t itr, int argc)
{
  int rc;

  if (argc)
    {
      fprintf (stderr, "ictl del?\n");
      return;
    }
  rc = mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
  if (rc)
    lperror ("mu_iterator_ctl", rc);
}

void
ictl_repl (mu_iterator_t itr, int argc, char **argv)
{
  int rc;
  
  if (argc != 1)
    {
      fprintf (stderr, "ictl repl item?\n");
      return;
    }

  rc = mu_iterator_ctl (itr, mu_itrctl_replace, strdup (argv[0]));
  if (rc)
    lperror ("mu_iterator_ctl", rc);
}

void
ictl_dir (mu_iterator_t itr, int argc, char **argv)
{
  int rc;
  int dir;
  
  if (argc > 1)
    {
      fprintf (stderr, "ictl dir [backwards|forwards]?\n");
      return;
    }
  if (argc == 1)
    {
      if (strcmp (argv[0], "backwards") == 0)
	dir = 1;
      else if (strcmp (argv[0], "forwards") == 0)
	dir = 0;
      else
	{
	  fprintf (stderr, "ictl dir [backwards|forwards]?\n");
	  return;
	}
      rc = mu_iterator_ctl (itr, mu_itrctl_set_direction, &dir);
      if (rc)
	lperror ("mu_iterator_ctl", rc);
    }
  else
    {
      rc = mu_iterator_ctl (itr, mu_itrctl_qry_direction, &dir);
      if (rc)
	lperror ("mu_iterator_ctl", rc);
      printf ("%s\n", dir ? "backwards" : "forwards");
    }
}
  
void
ictl_ins (mu_iterator_t itr, int argc, char **argv)
{
  int rc;
  
  if (argc < 1)
    {
      fprintf (stderr, "ictl ins item [item*]?\n");
      return;
    }

  if (argc == 1)
    rc = mu_iterator_ctl (itr, mu_itrctl_insert, strdup (argv[0]));
  else
    {
      mu_list_t tmp = read_list (argc, argv);
      if (!tmp)
	return;
      rc = mu_iterator_ctl (itr, mu_itrctl_insert_list, tmp);
      mu_list_destroy (&tmp);
    }
}

void
ictl (mu_iterator_t itr, int argc, char **argv)
{
  if (argc == 1)
    {
      fprintf (stderr, "ictl tell|del|repl|ins?\n");
      return;
    }
  
  if (strcmp (argv[1], "tell") == 0)
    ictl_tell (itr, argc - 2);
  else if (strcmp (argv[1], "del") == 0)
    ictl_del (itr, argc - 2);
  else if (strcmp (argv[1], "repl") == 0)
    ictl_repl (itr, argc - 2, argv + 2);
  else if (strcmp (argv[1], "ins") == 0)
    ictl_ins (itr, argc - 2, argv + 2);
  else if (strcmp (argv[1], "dir") == 0)
    ictl_dir (itr, argc - 2, argv + 2);
  else
    fprintf (stderr, "unknown subcommand\n");
}
    
#define NITR 4

int
iter (int *pnum, int argc, char **argv)
{
  int n;
  
  if (argc != 2)
    {
      fprintf (stderr, "iter num?\n");
      return 1;
    }

  n = strtoul (argv[1], NULL, 0);
  if (n < 0 || n >= NITR)
    {
      fprintf (stderr, "iter [0-3]?\n");
      return 1;
    }
  *pnum = n;
  return 0;
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
}

void
cur (int num, mu_iterator_t itr)
{
  char *text;
  size_t pos;
  int rc;

  printf ("%lu:", (unsigned long) num);
  rc = mu_iterator_ctl (itr, mu_itrctl_tell, &pos);
  if (rc == MU_ERR_NOENT)
    {
      printf ("iterator not initialized\n");
      return;
    }
  if (rc)
    lperror ("mu_iterator_ctl", rc);
  printf ("%lu:", (unsigned long) pos);

  rc = mu_iterator_current (itr, (void**) &text);
  if (rc)
    lperror ("mu_iterator_current", rc);
  printf ("%s\n", text);
}

static int
map_even (void **itmv, size_t itmc, void *call_data)
{
  int *num = call_data, n = *num;
  *num = !*num;
  if ((n % 2) == 0)
    {
      itmv[0] = strdup (itmv[0]);
      return MU_LIST_MAP_OK;
    }
  return MU_LIST_MAP_SKIP;
}

static int
map_odd (void **itmv, size_t itmc, void *call_data)
{
  int *num = call_data, n = *num;
  *num = !*num;
  if (n % 2)
    {
      itmv[0] = strdup (itmv[0]);
      return MU_LIST_MAP_OK;
    }
  return MU_LIST_MAP_SKIP;
}

static int
map_concat (void **itmv, size_t itmc, void *call_data)
{
  char *delim = call_data;
  size_t dlen = strlen (delim);
  size_t i;
  size_t len = 0;
  char *res, *p;
  
  for (i = 0; i < itmc; i++)
    len += strlen (itmv[i]);
  len += (itmc - 1) * dlen + 1;

  res = malloc (len);
  if (!res)
    abort ();
  p = res;
  for (i = 0; ; )
    {
      p = mu_stpcpy (p, itmv[i++]);
      if (i == itmc)
	break;
      p = mu_stpcpy (p, delim);
    }
  itmv[0] = res;
  return MU_LIST_MAP_OK;
}

struct trim_data
{
  size_t n;
  size_t lim;
};

static int
map_skip (void **itmv, size_t itmc, void *call_data)
{
  struct trim_data *td = call_data;

  if (td->n++ < td->lim)
    return MU_LIST_MAP_SKIP;
  itmv[0] = strdup (itmv[0]);
  return MU_LIST_MAP_OK;
}

static int
map_trim (void **itmv, size_t itmc, void *call_data)
{
  struct trim_data *td = call_data;

  if (td->n++ < td->lim)
    {
      itmv[0] = strdup (itmv[0]);
      return MU_LIST_MAP_OK;
    }
  return MU_LIST_MAP_STOP|MU_LIST_MAP_SKIP;
}

int
map (mu_list_t *plist, int argc, char **argv)
{
  mu_list_t list = *plist;
  mu_list_t result;
  int rc;
  int replace = 0;

  if (argc > 1 && strcmp (argv[1], "-replace") == 0)
    {
      replace = 1;
      argc--;
      argv++;
    }
  
  if (argc < 2)
    {
      fprintf (stderr, "map [-replace] even|odd|concat|keep|trim\n");
      return 0;
    }

  if (strcmp (argv[1], "even") == 0)
    {
      int n = 0;
      rc = mu_list_map (list, map_even, &n, 1, &result);
    }
  else if (strcmp (argv[1], "odd") == 0)
    {
      int n = 0;
      rc = mu_list_map (list, map_odd, &n, 1, &result);
    }
  else if (strcmp (argv[1], "concat") == 0)
    {
      size_t num;
      char *delim = "";
      
      if (argc < 3 || argc > 4)
	{
	  fprintf (stderr, "map concat NUM [DELIM]?\n");
	  return 0;
	}
      num = atoi (argv[2]);
      if (argc == 4)
	delim = argv[3];
      
      rc = mu_list_map (list, map_concat, delim, num, &result);
    }
  else if (strcmp (argv[1], "skip") == 0)
    {
      struct trim_data td;

      if (argc < 3 || argc > 4)
	{
	  fprintf (stderr, "map skip NUM?\n");
	  return 0;
	}
      td.n = 0;
      td.lim = atoi (argv[2]);
      rc = mu_list_map (list, map_skip, &td, 1, &result);
    }
  else if (strcmp (argv[1], "trim") == 0)
    {
      struct trim_data td;

      if (argc < 3 || argc > 4)
	{
	  fprintf (stderr, "map trim NUM?\n");
	  return 0;
	}
      td.n = 0;
      td.lim = atoi (argv[2]);
      rc = mu_list_map (list, map_trim, &td, 1, &result);
    }
  else
    {
      mu_error ("unknown map name\n");
      return 0;
    }
  
  if (rc)
    {
      mu_error ("map failed: %s", mu_strerror (rc));
      return 0;
    }

  mu_list_set_destroy_item (result, mu_list_free_item);

  if (replace)
    {
      size_t count[2];
      mu_list_count (list, &count[0]);
      mu_list_count (result, &count[1]);
      
      printf ("%lu in, %lu out\n", (unsigned long) count[0],
	      (unsigned long) count[1]);
      mu_list_destroy (&list);
      *plist = result;
      return 1;
    }
  else
    {
      print (result);
      mu_list_destroy (&result);
    }
  return 0;
}

static int
dup_string (void **res, void *itm, void *closure)
{
  *res = strdup (itm);
  return *res ? 0 : ENOMEM;
}

int
slice (mu_list_t *plist, int argc, char **argv)
{
  mu_list_t list = *plist;
  mu_list_t result;
  int rc, i;
  int replace = 0;
  size_t *buf;
  
  argc--;
  argv++;
  
  if (argc > 0 && strcmp (argv[0], "-replace") == 0)
    {
      replace = 1;
      argc--;
      argv++;
    }
  
  if (argc < 1)
    {
      fprintf (stderr, "slice [-replace] num [num...]\n");
      return 0;
    }

  buf = calloc (argc, sizeof (buf[0]));
  if (!buf)
    abort ();
  for (i = 0; i < argc; i++)
    buf[i] = atoi (argv[i]);

  rc = mu_list_slice_dup (&result, list, buf, argc,
			  dup_string, NULL);
  if (rc)
    {
      mu_error ("slice failed: %s", mu_strerror (rc));
      return 0;
    }
  if (replace)
    {
      size_t count[2];
      mu_list_count (list, &count[0]);
      mu_list_count (result, &count[1]);
      
      printf ("%lu in, %lu out\n", (unsigned long) count[0],
	      (unsigned long) count[1]);
      mu_list_destroy (&list);
      *plist = result;
      return 1;
    }
  else
    {
      print (result);
      mu_list_destroy (&result);
    }
  return 0;  
}

void
head (size_t argc, mu_list_t list)
{
  int rc;
  const char *text;
    
  if (argc != 1)
    {
      fprintf (stderr, "head ?\n");
      return;
    }
  rc = mu_list_head (list, (void**) &text);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_list_head", NULL, rc);
  else
    printf ("%s\n", text);
}

void
tail (size_t argc, mu_list_t list)
{
  int rc;
  const char *text;
    
  if (argc != 1)
    {
      fprintf (stderr, "head ?\n");
      return;
    }
  rc = mu_list_tail (list, (void**) &text);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_list_tail", NULL, rc);
  else
    printf ("%s\n", text);
}

static int
fold_concat (void *item, void *data, void *prev, void **ret)
{
  char *s;
  size_t len = strlen (item);
  size_t prevlen = 0;
  
  if (prev)
    prevlen = strlen (prev);

  s = realloc (prev, len + prevlen + 1);
  if (!s)
    abort ();
  strcpy (s + prevlen, item);
  *ret = s;
  return 0;
}

void
fold (mu_list_t list)
{
  char *text = NULL;
  int rc;

  rc = mu_list_fold (list, fold_concat, NULL, NULL, &text);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_list_fold", NULL, rc);
  else if (text)
    {
      printf ("%s\n", text);
      free (text);
    }
  else
    printf ("NULL\n");
}

void
rfold (mu_list_t list)
{
  char *text = NULL;
  int rc;

  rc = mu_list_rfold (list, fold_concat, NULL, NULL, &text);
  if (rc)
    mu_diag_funcall (MU_DIAG_ERROR, "mu_list_fold", NULL, rc);
  else if (text)
    {
      printf ("%s\n", text);
      free (text);
    }
  else
    printf ("NULL\n");
}

void
sort (mu_list_t list)
{
  mu_list_sort (list, NULL);
}

void
push (mu_list_t list, int argc, char **argv)
{
  if (argc < 2)
    {
      fprintf (stderr, "push arg [arg] ?\n");
      return;
    }

  while (--argc)
    {
      int rc = mu_list_push (list, strdup (*++argv));
      if (rc)
	fprintf (stderr, "mu_list_push: %s\n", mu_strerror (rc));
    }
}

void
pop (mu_list_t list, int argc, char **argv)
{
  char *text;
  int rc;
  
  if (argc != 1)
    {
      fprintf (stderr, "pop ?\n");
      return;
    }

  rc = mu_list_pop (list, (void**) &text);
  if (rc)
    fprintf (stderr, "mu_list_pop: %s\n", mu_strerror (rc));
  else
    printf ("%s\n", text);
}
  
void
help ()
{
  printf ("count\n");
  printf ("cur\n");
  printf ("next [count]\n");
  printf ("first\n");
  printf ("find item\n");
  printf ("del item [item*]\n");
  printf ("add item [item*]\n");
  printf ("prep item [item*]\n");
  printf ("repl old_item new_item\n");
  printf ("ins [before|after] item new_item [new_item*]\n");
  printf ("ictl tell\n");
  printf ("ictl del\n");
  printf ("ictl repl item\n");
  printf ("ictl ins item [item*]\n");
  printf ("ictl dir [backwards|forwards]\n");
  printf ("map [-replace] NAME [ARGS]\n");
  printf ("fold\n");
  printf ("rfold\n");
  printf ("print\n");
  printf ("slice [-replace] num [num...]\n");
  printf ("quit\n");
  printf ("iter num\n");
  printf ("help\n");
  printf ("head\n");
  printf ("tail\n");
  printf ("push item\n");
  printf ("pop\n");
  printf ("sort\n");
  printf ("NUMBER\n");
}

void
shell (mu_list_t list)
{
  int num = 0;
  mu_iterator_t itr[NITR];
  int rc;

  memset (&itr, 0, sizeof itr);
  num = 0;
  while (1)
    {
      char *text = NULL;
      char buf[80];
      struct mu_wordsplit ws;
      
      if (!itr[num])
	{
	  rc = mu_list_get_iterator (list, &itr[num]);
	  if (rc)
	    lperror ("mu_list_get_iterator", rc);
	  mu_iterator_first (itr[num]);
	}
      
      rc = mu_iterator_current (itr[num], (void**) &text);
      if (rc)
	lperror ("mu_iterator_current", rc);

      if (interactive)
	printf ("%d:(%s)> ", num, text ? text : "NULL");
      if (fgets (buf, sizeof buf, stdin) == NULL)
	return;

      ws.ws_comment = "#";
      if (mu_wordsplit (buf, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_COMMENT))
	{
	  mu_error ("cannot split line `%s': %s", buf,
		    mu_wordsplit_strerror (&ws));
	  exit (1);
	}

      if (ws.ws_wordc > 0)
	{
	  if (strcmp (ws.ws_wordv[0], "count") == 0)
	    count (list);
	  else if (strcmp (ws.ws_wordv[0], "next") == 0)
	    next (itr[num], ws.ws_wordv[1]);
	  else if (strcmp (ws.ws_wordv[0], "first") == 0)
	    mu_iterator_first (itr[num]);
	  else if (strcmp (ws.ws_wordv[0], "del") == 0)
	    delete (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "add") == 0)
	    add (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "prep") == 0)
	    prep (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "ins") == 0)
	    ins (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "repl") == 0)
	    repl (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "ictl") == 0)
	    ictl (itr[num], ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "print") == 0)
	    print (list);
	  else if (strcmp (ws.ws_wordv[0], "cur") == 0)
	    cur (num, itr[num]);
	  else if (strcmp (ws.ws_wordv[0], "fold") == 0)
	    fold (list);
	  else if (strcmp (ws.ws_wordv[0], "rfold") == 0)
	    rfold (list);
	  else if (strcmp (ws.ws_wordv[0], "map") == 0)
	    {
	      int i;
	      
	      if (map (&list, ws.ws_wordc, ws.ws_wordv))
		for (i = 0; i < NITR; i++)
		  mu_iterator_destroy (&itr[i]);
	    }
	  else if (strcmp (ws.ws_wordv[0], "slice") == 0)
	    {
	      int i;
	      
	      if (slice (&list, ws.ws_wordc, ws.ws_wordv))
		for (i = 0; i < NITR; i++)
		  mu_iterator_destroy (&itr[i]);
	    }
	  else if (strcmp (ws.ws_wordv[0], "quit") == 0)
	    return;
	  else if (strcmp (ws.ws_wordv[0], "iter") == 0)
	    {
	      int n;
	      if (iter (&n, ws.ws_wordc, ws.ws_wordv) == 0 && !itr[n])
		{
		  rc = mu_list_get_iterator (list, &itr[n]);
		  if (rc)
		    lperror ("mu_list_get_iterator", rc);
		  mu_iterator_first (itr[n]);
		}
	      num = n;
	    }
	  else if (strcmp (ws.ws_wordv[0], "close") == 0)
	    {
	      int n;
	      if (iter (&n, ws.ws_wordc, ws.ws_wordv) == 0)
		{
		  mu_iterator_destroy (&itr[n]);
		  if (n == num && ++num == NITR)
		    num = 0;
		}
	    }
	  else if (strcmp (ws.ws_wordv[0], "find") == 0)
	    find (itr[num], ws.ws_wordv[1]);
	  else if (strcmp (ws.ws_wordv[0], "help") == 0)
	    help ();
	  else if (strcmp (ws.ws_wordv[0], "head") == 0)
	    head (ws.ws_wordc, list);
	  else if (strcmp (ws.ws_wordv[0], "tail") == 0)
	    tail (ws.ws_wordc, list);
	  else if (strcmp (ws.ws_wordv[0], "push") == 0)
	    push (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "pop") == 0)
	    pop (list, ws.ws_wordc, ws.ws_wordv);
	  else if (strcmp (ws.ws_wordv[0], "sort") == 0)
	    {
	      int i;
	      sort (list);

	      for (i = 0; i < NITR; i++)
		mu_iterator_destroy (&itr[i]);
	    }
	  else if (ws.ws_wordc == 1)
	    {
	      char *p;
	      size_t n = strtoul (ws.ws_wordv[0], &p, 0);
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
      mu_wordsplit_free (&ws);
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

  interactive = isatty (0);
  rc = mu_list_create (&list);
  if (rc)
    lperror ("mu_list_create", rc);
  mu_list_set_comparator (list, string_comp);
  mu_list_set_destroy_item (list, mu_list_free_item);

  argc--;
  argv++;
  
  while (argc--)
    {
      rc = mu_list_append (list, strdup (*argv++));
      if (rc)
	lperror ("mu_list_append", rc);
    }

  shell (list);
  
  return 0;
}
