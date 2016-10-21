/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2012, 2014-2016 Free Software Foundation, Inc.

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

/* MH ali command */

#include <mh.h>
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>

static char prog_doc[] = N_("GNU MH ali");
static char args_doc[] = N_("ALIAS [ALIAS...]");

static int list_mode;
static int user_mode;
static int normalize_mode;
static int nolist_mode;

static void
alias_handler (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  mh_alias_read (arg, 1);
}

static struct mu_option options[] = {
  { "alias",  0, N_("FILE"), MU_OPTION_DEFAULT,
    N_("use the additional alias FILE"),
    mu_c_string, NULL, alias_handler },
  { "noalias", 0, NULL, 0,
    N_("do not read the system alias file"),
    mu_c_int, &nolist_mode, NULL, "1" },
  { "list", 0, NULL,  MU_OPTION_DEFAULT,
    N_("list each address on a separate line"),
    mu_c_bool, &list_mode },
  { "normalize", 0, NULL, MU_OPTION_DEFAULT,
    N_("try to determine the official hostname for each address"),
    mu_c_bool, &normalize_mode },
  { "user", 0, NULL,  MU_OPTION_DEFAULT,
    N_("list the aliases that expand to given addresses"),
    mu_c_bool, &user_mode },
  MU_OPTION_END
};

static int
getcols (void)
{
  struct winsize ws;

  ws.ws_col = ws.ws_row = 0;
  if ((ioctl(1, TIOCGWINSZ, (char *) &ws) < 0) || ws.ws_row == 0)
    {
      const char *columns = getenv ("COLUMNS");
      if (columns)
	ws.ws_col = strtol (columns, NULL, 10);
    }

  if (ws.ws_col == 0)
    ws.ws_col = 80;
  return ws.ws_col;
}

static void
ali_print_name_list (mu_list_t list, int off)
{
  mu_iterator_t itr;
  char *item;
  
  mu_list_get_iterator (list, &itr);
  
  if (list_mode)
    {
      for (mu_iterator_first (itr);
	   !mu_iterator_is_done (itr); mu_iterator_next (itr))
	{
	  mu_iterator_current (itr, (void **)&item);
	  printf ("%s\n", item);
	}
    }
  else
    {
      int ncol = getcols ();
      int n = off;
      int i = 0;
      
      for (mu_iterator_first (itr), i = 0;
	   !mu_iterator_is_done (itr);
	   mu_iterator_next (itr), i++)
	{
	  int len;

	  if (i > 0)
	    n += printf (", ");
	  
	  mu_iterator_current (itr, (void **)&item);
	  len = strlen (item) + 2;
	  if (n + len > ncol)
	    n = printf ("\n ");

	  len = printf ("%s", item);
	  n += len;
	}
      printf ("\n");
    }
  mu_iterator_destroy (&itr);
}

static int
ali_print_alias (char *name, mu_list_t alias, void *data MU_ARG_UNUSED)
{
  int n;
  
  if (name)
    n = printf ("%s: ", name);
  else
    n = 0;

  ali_print_name_list (alias, n);
  return 0;
}

static void
ali_print_name (char *name)
{
  printf ("%s\n", name);
}

int
main (int argc, char **argv)
{
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mh_getopt (&argc, &argv, options, 0, args_doc, prog_doc, NULL);

  if (!nolist_mode)
    mh_read_aliases ();

  if (!user_mode)
    {
      if (argc == 0)
	mh_alias_enumerate (ali_print_alias, NULL);
      else
	{
	  int i;
	  for (i = 0; i < argc; i++)
	    {
	      mu_list_t al = NULL;
	      
	      if (mh_alias_get (argv[i], &al) == 0)
		{
		  ali_print_alias (NULL, al, NULL);
		  mu_list_destroy (&al);
		}
	      else
		ali_print_name (argv[i]);
	    }
	}
    }
  else
    {
      if (argc == 0)
	{
	  mu_error ("List of addresses is not given");
	  exit (1);
	}
      else
	{
	  int i;
	  for (i = 0; i < argc; i++)
	    {
	      mu_list_t nl = NULL;

	      if (mh_alias_get_alias (argv[i], &nl) == 0)
		{
		  ali_print_name_list (nl, 0);
		  mu_list_destroy (&nl);
		}
	      else
		printf ("%s\n", argv[i]);
	    }
	}
    }
  return 0;
}
