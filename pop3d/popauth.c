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

#include "pop3d.h"

static char short_options[] = "f:hlo:v";

static struct option long_options[] =
{
  { "file", required_argument, 0, 'f' },
  { "help", no_argument, 0, 'h' },
  { "list", no_argument, 0, 'l' },
  { "output", required_argument, 0, 'o' },
  { "version", no_argument, 0, 'v', },
  { 0, 0, 0, 0 }
};

int db_list (char *input_name, char *output_name);
int db_make (char *input_name, char *output_name);
static void help (void);
static void version (void);

int
main(int argc, char **argv)
{
  int c;
  int list = 0;
  char *input_name = NULL;
  char *output_name = NULL;
  
  while ((c = getopt_long (argc, argv, short_options, long_options, NULL))
	 != -1)
    {
      switch (c)
	{
	case 'f':
	  input_name = optarg;
	  break;
	case 'h':
	  help ();
	  break;
	case 'l':
	  list = 1;
	  break;
	case 'o':
	  output_name = optarg;
	  break;
	case 'v':
	  version ();
	  break;
	default:
	  break;
	}
    }

  if (list)
    return db_list (input_name, output_name);

  return db_make (input_name, output_name);
}


int
db_list (char *input_name, char *output_name)
{
  FILE *fp;
  DBM_FILE db;
  DBM_DATUM key;
  DBM_DATUM contents;
  
  if (!input_name)
    input_name = APOP_PASSFILE;
  if (mu_dbm_open (input_name, &db, MU_STREAM_READ, 0600))
    {
      mu_error("can't open %s: %s", input_name, strerror (errno));
      return 1;
    }
  
  if (output_name)
    {
      fp = fopen (output_name, "w");
      if (!fp)
	{
	  mu_error("can't create %s: %s", output_name, strerror (errno));
	  return 1;
	}
    }
  else
    fp = stdout;

  for (key = mu_dbm_firstkey (db); MU_DATUM_PTR(key);
       key = mu_dbm_nextkey (db, key))
    {
      mu_dbm_fetch (db, key, &contents);
      fprintf (fp, "%.*s: %.*s\n",
	       (int) MU_DATUM_SIZE (key),
	       (char*) MU_DATUM_PTR (key),
	       (int) MU_DATUM_SIZE (contents),
	       (char*) MU_DATUM_PTR (contents));
    }
  mu_dbm_close (db);
  fclose (fp);
  return 0;
}

int
db_make (char *input_name, char *output_name)
{
  FILE *fp;
  DBM_FILE db;
  DBM_DATUM key;
  DBM_DATUM contents;
  char buf[256];
  int line = 0;
  
  if (input_name)
    {
      fp = fopen (input_name, "r");
      if (!fp)
	{
	  mu_error("can't create %s: %s", input_name, strerror (errno));
	  return 1;
	}
    }
  else
    {
      input_name = "";
      fp = stdin;
    }
  
  if (!output_name)
    output_name = APOP_PASSFILE;
  if (mu_dbm_open (output_name, &db, MU_STREAM_CREAT, 0600))
    {
      mu_error("can't create %s: %s", output_name, strerror (errno));
      return 1;
    }

  line = 0;
  while (fgets (buf, sizeof buf - 1, fp))
    {
      int len;
      int argc;
      char **argv;

      len = strlen (buf);
      if (buf[len-1] == '\n')
	buf[--len] = 0;
      
      line++;
      if (argcv_get (buf, ":", &argc, &argv))
	{
	  argcv_free (argc, argv);
	  continue;
	}

      if (argc == 0 || argv[0][0] == '#')
	{
	  argcv_free (argc, argv);
	  continue;
	}
      
      if (argc != 3 || argv[1][0] != ':' || argv[1][1] != 0)
	{
	  mu_error ("%s:%d: malformed line", input_name, line);
	  argcv_free (argc, argv);
	  continue;
	}

      memset (&key, 0, sizeof key);
      memset (&contents, 0, sizeof contents);
      MU_DATUM_PTR (key) = argv[0];
      MU_DATUM_SIZE (key) = strlen (argv[0]);
      MU_DATUM_PTR (contents) = argv[2];
      MU_DATUM_SIZE (contents) = strlen (argv[2]);

      if (mu_dbm_insert (db, key, contents, 1))
	mu_error ("%s:%d: can't store datum", input_name, line);

      argcv_free (argc, argv);
    }
  mu_dbm_close (db);
  fclose (fp);
  return 0;
}

static void
help ()
{
  printf ("Usage: popauth [OPTIONS]\n");
  printf ("Manipulates pop3d authentication database.\n\n");
  printf ("  -f, --file=FILE          Read input from FILE (default stdin)\n");
  printf ("  -o, --output=FILE        Direct output to FILE\n");
  printf ("  -l, --list               List the contents of DBM file\n");
  printf ("  -h, --help               Display this help and exit\n");
  printf ("  -v, --version            Display program version\n");
  printf ("\nDefault action is to convert plaintext to DBM file.\n");
  printf ("\nReport bugs to bug-mailutils@gnu.org\n");
  exit (EXIT_SUCCESS);
}

static void
version ()
{
#if defined(WITH_GDBM)
# define FORMAT "GDBM"
#elif defined(WITH_BDB2)
# define FORMAT "Berkeley DB"  
#elif defined(WITH_NDBM)
# define FORMAT "NDBM"  
#elif defined(WITH_OLD_DBM)
# define FORMAT "Old DBM"  
#endif
  printf ("popauth: %s (%s)\n", PACKAGE, VERSION);
  printf ("Database format: %s\n", FORMAT);
  printf ("Database location: %s\n", APOP_PASSFILE);
  exit (EXIT_SUCCESS);
}
