/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "guimb.h"
#include "getopt.h"

static char short_options[] = "de:f:g:hm:v";
static struct option long_options[] = {
  {"debug", no_argument, 0, 'd'},
  {"expression", required_argument, 0, 'e'},
  {"file", required_argument, 0, 'f'},
  {"help", no_argument, 0, 'h'},
  {"guile-command", required_argument, 0, 'g'},
  {"version", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

char *program_file;
char *program_expr;
int debug_guile;

static void usage (void);

static int g_size;
static int g_argc;
static char **g_argv;

#define ARG_INC 16

void
append_arg (char *arg)
{
  if (g_argc == g_size)
    {
      g_size += ARG_INC;
      g_argv = realloc (g_argv, g_size * sizeof (g_argv[0]));
      if (!g_argv)
	{
	  util_error ("not enough memory");
	  exit (1);
	}
    }
  g_argv[g_argc++] = arg;
}

int
main (int argc, char *argv[])
{
  int c;

  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    switch (c)
      {
      case 'd':
	debug_guile = 1;
	break;
      case 'f':
	program_file = optarg;
	break;
      case 'e':
	program_expr = optarg;
	break;
      case 'g':
	append_arg (optarg);
	break;
      case 'h':
	usage ();
	exit (0);
      case 'v':
	printf ("guimb (" PACKAGE " " VERSION ")\n");
	exit (0);
      default:
	fprintf (stderr,
		 "Invalid argument. Try guimb --help for more info\n");
	exit (1);
      }
  append_arg (NULL);
  g_argc--;

  /* Register the desired formats. */
  {
    list_t lst;
    registrar_get_list (&lst);
    list_append (lst, mbox_record);
    list_append (lst, path_record);
    list_append (lst, pop_record);
    list_append (lst, imap_record);
    /* Possible supported mailers.  */
    list_append (lst, sendmail_record);
    list_append (lst, smtp_record);
  }

  collect_open_mailbox_file ();
  if (argv[optind])
    {
      for (; argv[optind]; optind++)
	collect_append_file (argv[optind]);
    }
  else
    collect_append_file ("-");
  run_main (g_argc, g_argv);
}

static char usage_str[] =
  "Usage: guimb [OPTIONS] [MBOX ...]\n"
  "Process the contents of the specified mailboxes using a Scheme program\n"
  "or expression.\n"
  "Options are:\n"
  "  -d, --debug               Start with debugging evaluator and backtraces.\n"
  "  -e, --expression EXPR     Execute scheme expression.\n"
  "  -f, --file PROGFILE       Read program from PROGFILE.\n"
  "  -g, --guile-command ARG   Append ARG to the command line passed to guile\n"
  "                            program.\n"
  "  -h, --help                Display help message.\n"
  "  -v, --version             Display program version.\n"
  "\n"
  "When both --file and --expression are specified, file is evaluated first.\n"
  "If no mailboxes are specified the standard input is read.\n";

void
usage ()
{
  printf ("%s\n" "Report bugs to <bug-mailutils@gnu.org>\n", usage_str);
}
