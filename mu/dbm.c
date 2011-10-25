/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>
#include <sysexits.h>
#include <fnmatch.h>
#include <regex.h>
#include <mailutils/mailutils.h>
#include <mailutils/dbm.h>
#include "argp.h"
#include "mu.h"
#include "xalloc.h"

static char dbm_doc[] = N_("mu dbm - DBM management tool");
char dbm_docstring[] = N_("DBM management tool");
static char dbm_args_doc[] = N_("FILE [KEY...]");

enum mode
  {
    mode_list,
    mode_create,
    mode_delete,
    mode_add,
    mode_replace,
  };

enum key_type
  {
    key_literal,
    key_glob,
    key_regex
  };

enum mode mode;
char *db_name;
char *input_file;
int permissions = 0600;
struct mu_auth_data *auth;
uid_t owner_uid = -1;
gid_t owner_gid = -1;
int copy_permissions;
enum key_type key_type = key_literal;
int case_sensitive = 1;
int include_zero = -1;

void
init_datum (struct mu_dbm_datum *key, char *str)
{
  memset (key, 0, sizeof *key);
  key->mu_dptr = str;
  key->mu_dsize = strlen (str) + !!include_zero;
}

mu_dbm_file_t
open_db_file (int mode)
{
  int rc;
  mu_dbm_file_t db;
  
  rc = mu_dbm_create (db_name, &db);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to create database %s: %s"),
		      db_name, mu_strerror (rc));
      exit (EX_SOFTWARE);
    }

  rc = mu_dbm_open (db, mode, permissions);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_open", db_name, rc);
      exit (EX_SOFTWARE);
    }

  if (mode == MU_STREAM_CREAT && (owner_uid != -1 || owner_gid != -1))
    {
      int dirfd, pagfd;
      
      rc = mu_dbm_get_fd (db, &pagfd, &dirfd);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_get_fd", db_name, rc);
	  exit (EX_SOFTWARE);
	}
      if (owner_uid == -1 || owner_gid == -1)
	{
	  struct stat st;
	  
	  if (fstat (dirfd, &st))
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "fstat", db_name, errno);
	      exit (EX_SOFTWARE);
	    }
	  if (owner_uid == -1)
	    owner_uid = st.st_uid;
	  if (owner_gid == -1)
	    owner_gid = st.st_gid;
	}
      if (fchown (pagfd, owner_uid, owner_gid))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "fchown", db_name, errno);
	  exit (EX_SOFTWARE);
	}
      if (pagfd != dirfd &&
	  fchown (dirfd, owner_uid, owner_gid))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "fchown", db_name, errno);
	  exit (EX_SOFTWARE);
	}
    }
  return db;
}

struct print_data
{
  mu_dbm_file_t db;
  mu_stream_t stream;
};
  
static int
print_action (struct mu_dbm_datum const *key, void *data)
{
  int rc;
  struct mu_dbm_datum contents;
  struct print_data *pd = data;
  size_t len;
  
  memset (&contents, 0, sizeof contents);
  rc = mu_dbm_fetch (pd->db, key, &contents);
  if (rc)
    {
      mu_error (_("database fetch error: %s"), mu_dbm_strerror (pd->db));
      exit (EX_UNAVAILABLE);
    }

  len = key->mu_dsize;
  if (key->mu_dptr[len - 1] == 0)
    len--;
  mu_stream_write (pd->stream, key->mu_dptr, len, NULL);
  mu_stream_write (pd->stream, "\t", 1, NULL);

  len = contents.mu_dsize;
  if (contents.mu_dptr[len - 1] == 0)
    len--;
  mu_stream_write (pd->stream, contents.mu_dptr, len, NULL);
  mu_stream_write (pd->stream, "\n", 1, NULL);
  mu_dbm_datum_free (&contents);
  return 0;
}

static void
iterate_database (mu_dbm_file_t db,
		  int (*matcher) (const char *, void *), void *matcher_data,
		  int (*action) (struct mu_dbm_datum const *, void *),
		  void *action_data)
{
  int rc;
  struct mu_dbm_datum key;
  char *buf = NULL;
  size_t bufsize = 0;

  memset (&key, 0, sizeof key);
  for (rc = mu_dbm_firstkey (db, &key); rc == 0;
       rc = mu_dbm_nextkey (db, &key))
    {
      if (include_zero == -1)
	include_zero = key.mu_dptr[key.mu_dsize-1] == 0;

      if (matcher)
	{
	  if (key.mu_dsize + 1 > bufsize)
	    {
	      bufsize = key.mu_dsize + 1;
	      buf = xrealloc (buf, bufsize);
	    }
	  memcpy (buf, key.mu_dptr, key.mu_dsize);
	  buf[key.mu_dsize] = 0;
	  if (!matcher (buf, matcher_data))
	    continue;
	}
      if (action (&key, action_data))
	break;
    }
  free (buf);
}

static int
match_literal (const char *str, void *data)
{
  char **argv = data;

  for (; *argv; argv++)
    {
      if ((case_sensitive ? strcmp : strcasecmp) (str, *argv) == 0)
	return 1;
    }
  return 0;
}

#ifndef FNM_CASEFOLD
# define FNM_CASEFOLD 0
#endif

static int
match_glob (const char *str, void *data)
{
  char **argv = data;
  
  for (; *argv; argv++)
    {
      if (fnmatch (*argv, str, case_sensitive ? FNM_CASEFOLD : 0) == 0)
	return 1;
    }
  return 0;
}

struct regmatch
{
  int regc;
  regex_t *regs;
};
  
static int
match_regex (const char *str, void *data)
{
  struct regmatch *match = data;
  int i;
  
  for (i = 0; i < match->regc; i++)
    {
      if (regexec (&match->regs[i], str, 0, NULL, 0) == 0)
	return 1;
    }
  return 0;
}

void
compile_regexes (int argc, char **argv, struct regmatch *match)
{
  regex_t *regs = xcalloc (argc, sizeof (regs[0]));
  int i;
  int cflags = (case_sensitive ? 0: REG_ICASE) | REG_EXTENDED | REG_NOSUB;
  int errors = 0;
  
  for (i = 0; i < argc; i++)
    {
      int rc = regcomp (&regs[i], argv[i], cflags);
      if (rc)
	{
	  char errbuf[512];
	  regerror (rc, &regs[i], errbuf, sizeof (errbuf));
	  mu_error (_("error compiling `%s': %s"), argv[i], errbuf);
	  errors++;
	}
    }
  if (errors)
    exit (EX_USAGE);
  match->regc = argc;
  match->regs = regs;
}

static void
free_regexes (struct regmatch *match)
{
  int i;
  for (i = 0; i < match->regc; i++)
    regfree (&match->regs[i]);
  free (match->regs);
}

static void
list_database (int argc, char **argv)
{
  mu_dbm_file_t db = open_db_file (MU_STREAM_READ);
  struct print_data pdata;

  pdata.db = db;
  pdata.stream = mu_strout;

  if (argc == 0)
    iterate_database (db, NULL, NULL, print_action, &pdata);
  else
    {
      switch (key_type)
	{
	case key_literal:
	  iterate_database (db, match_literal, argv, print_action, &pdata);
	  break;

	case key_glob:
	  iterate_database (db, match_glob, argv, print_action, &pdata);
	  break;

	case key_regex:
	  {
	    struct regmatch m;
	    compile_regexes (argc, argv, &m);
	    iterate_database (db, match_regex, &m, print_action, &pdata);
	    free_regexes (&m);
	  }
	}
    }
  mu_dbm_destroy (&db);
}

void
add_records (int mode, int replace)
{
  mu_dbm_file_t db;
  mu_stream_t input, flt;
  const char *flt_argv[] = { "inline-comment", "#", "-S", "-i", "#", NULL };
  char *buf = NULL;
  size_t size = 0;
  size_t len;
  unsigned long line;
  int rc;
  
  if (input_file)
    {
      rc = mu_file_stream_create (&input, input_file, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open input file %s: %s"), input_file,
		    mu_strerror (rc));
	  exit (EX_UNAVAILABLE);
	}
    }
  else
    input = mu_strin;
  
  rc = mu_filter_create_args (&flt, input, "inline-comment",
			      MU_ARRAY_SIZE (flt_argv) - 1, flt_argv,
			      MU_FILTER_DECODE, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_filter_create_args", input_file, rc);
      exit (EX_UNAVAILABLE);
    }
  mu_stream_unref (input);
  input = flt;

  db = open_db_file (mode);

  line = 1;
  if (!input_file)
    input_file = "<stdin>";
  while ((rc = mu_stream_getline (input, &buf, &size, &len)) == 0
	 && len > 0)
    {
      struct mu_dbm_datum key, contents;
      char *kstr, *val;
      
      mu_rtrim_class (buf, MU_CTYPE_ENDLN);
      if (buf[0] == '#')
	{
	  line = strtoul (buf + 1, NULL, 10);
	  continue;
	}
      kstr = mu_str_stripws (buf);
      val = mu_str_skip_class_comp (kstr, MU_CTYPE_SPACE);
      *val++ = 0;
      val = mu_str_skip_class (val, MU_CTYPE_SPACE);
      if (*val == 0)
	{
	  mu_error (_("%s:%lu: malformed line"), input_file, line);
	  line++;
	  continue;
	}

      init_datum (&key, kstr);
      init_datum (&contents, val);

      rc = mu_dbm_store (db, &key, &contents, replace);
      if (rc)
	mu_error (_("%s:%lu: cannot store datum: %s"),
		  input_file, line,
		  rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));
      line++;
    }
  mu_dbm_destroy (&db);

  mu_stream_unref (input);
}

static void
create_database ()
{
  if (copy_permissions)
    {
      struct stat st;
      
      if (!input_file)
	{
	  mu_error (_("--copy-permissions used without --file"));
	  exit (EX_USAGE);
	}
      if (stat (input_file, &st))
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "stat", input_file, errno);
	  exit (EX_UNAVAILABLE);
	}
      owner_uid = st.st_uid;
      owner_gid = st.st_gid;
      permissions = st.st_mode & 0777;
    }
  else if (auth)
    {
      if (owner_uid == -1)
	owner_uid = auth->uid;
      if (owner_gid == -1)
	owner_gid = auth->gid;
    }
  add_records (MU_STREAM_CREAT, 0);
}

static int
store_to_list (struct mu_dbm_datum const *key, void *data)
{
  int rc;
  mu_list_t list = data;
  char *p = xmalloc (key->mu_dsize + 1);
  memcpy (p, key->mu_dptr, key->mu_dsize);
  p[key->mu_dsize] = 0;
  rc = mu_list_append (list, p);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", p, rc);
      exit (EX_SOFTWARE);
    }
  return 0;
}

static int
do_delete (void *item, void *data)
{
  char *str = item;
  mu_dbm_file_t db = data;
  struct mu_dbm_datum key;
  int rc;

  init_datum (&key, str);
  rc = mu_dbm_delete (db, &key);
  if (rc == MU_ERR_NOENT)
    {
      mu_error (_("cannot remove record for %s: %s"),
		str, mu_strerror (rc));
    }
  else if (rc)
    {
      mu_error (_("cannot remove record for %s: %s"),
		str, mu_dbm_strerror (db));
      if (rc != MU_ERR_NOENT)
	exit (EX_UNAVAILABLE);
    }
  return 0;
}

static void
delete_database (int argc, char **argv)
{
  mu_dbm_file_t db = open_db_file (MU_STREAM_RDWR);
  mu_list_t templist = NULL;
  int rc, i;
  
  if (argc == 0)
    {
      mu_error (_("not enough arguments for delete"));
      exit (EX_USAGE);
    }

  /* Collect matching keys */
  rc = mu_list_create (&templist);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_create", NULL, rc);
      exit (EX_SOFTWARE);
    }
  mu_list_set_destroy_item (templist, mu_list_free_item);
  switch (key_type)
    {
    case key_literal:
      for (i = 0; i < argc; i++)
	{
	  char *p = xstrdup (argv[i]);
	  rc = mu_list_append (templist, p);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_list_append", p, rc);
	      exit (EX_SOFTWARE);
	    }
	}
      break;

    case key_glob:
      iterate_database (db, match_glob, argv,
			store_to_list, templist);
      break;

    case key_regex:
      {
	struct regmatch m;
	
	compile_regexes (argc, argv, &m);
	iterate_database (db, match_regex, &m, store_to_list, templist);
	free_regexes (&m);
      }
    }
  mu_list_do (templist, do_delete, db);
  mu_list_destroy (&templist);
  mu_dbm_destroy (&db);
}

/*
  mu dbm --create a.db < input
  mu dbm --list a.db
  mu dbm --delete a.db key [key...]
  mu dbm --add a.db < input
  mu dbm --replace a.db < input
 */

static struct argp_option dbm_options[] = {
  { NULL, 0, NULL, 0, N_("Operation mode"), 0},
  { "create", 'c', NULL, 0, N_("create the database") },
  { "list",   'l', NULL, 0, N_("list contents of the database") },
  { "delete", 'd', NULL, 0, N_("delete specified keys from the database") },
  { "add",    'a', NULL, 0, N_("add records to the database") },
  { "replace",'r', NULL, 0, N_("add records replacing ones with matching keys") },
  
  { NULL, 0, NULL, 0, N_("Create modifiers"), 0},
  { "file",   'f', N_("FILE"), 0,
    N_("read input from FILE (with --create, --delete, --add and --replace)") },
  { "permissions", 'p', N_("NUM"), 0,
    N_("set permissions on the created database") },
  { "user",        'u', N_("USER"), 0,
    N_("set database owner name") },
  { "group",       'g', N_("GROUP"), 0,
    N_("set database owner group") },
  { "copy-permissions", 'P', NULL, 0,
    N_("copy database permissions and ownership from the input file") },
  { NULL, 0, NULL, 0, N_("List and Delete modifiers"), 0},
  { "glob",        'G', NULL, 0,
    N_("treat keys as globbing patterns") },
  { "regex",       'R', NULL, 0,
    N_("treat keys as regular expressions") },
  { "ignore-case", 'i', NULL, 0,
    N_("case-insensitive matches") },
  { NULL, 0, NULL, 0, N_("Data length modifiers"), 0 },
  { "count-null",  'N', NULL, 0,
    N_("data length accounts for terminating zero") },
  { "no-count-null", 'n', NULL, 0,
    N_("data length does not account for terminating zero") },
  { NULL }
};

static error_t
dbm_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'c':
      mode = mode_create;
      break;

    case 'l':
      mode = mode_list;
      break;

    case 'd':
      mode = mode_delete;
      break;

    case 'a':
      mode = mode_add;
      break;

    case 'r':
      mode = mode_replace;
      break;

    case 'f':
      input_file = arg;
      break;

    case 'p':
      {
	char *p;
	unsigned long d = strtoul (arg, &p, 8);
	if (*p || (d & ~0777))
	  argp_error (state, _("invalid file mode: %s"), arg);
	permissions = d;
      }
      break;

    case 'P':
      copy_permissions = 1;
      break;

    case 'u':
      auth = mu_get_auth_by_name (arg);
      if (!auth)
	{
	  char *p;
	  unsigned long n = strtoul (arg, &p, 0);
	  if (*p == 0)
	    owner_uid = n;
	  else
	    argp_error (state, _("no such user: %s"), arg);
	}
      break;

    case 'g':
      {
	struct group *gr = getgrnam (arg);
	if (!gr)
	  {
	    char *p;
	    unsigned long n = strtoul (arg, &p, 0);
	    if (*p == 0)
	      owner_gid = n;
	    else
	      argp_error (state, _("no such group: %s"), arg);
	  }
	else
	  owner_gid = gr->gr_gid;
      }
      break;

    case 'G':
      key_type = key_glob;
      break;

    case 'R':
      key_type = key_regex;
      break;

    case 'i':
      case_sensitive = 0;
      break;

    case 'N':
      include_zero = 1;
      break;

    case 'n':
      include_zero = 0;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp dbm_argp = {
  dbm_options,
  dbm_parse_opt,
  dbm_args_doc,
  dbm_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_dbm (int argc, char **argv)
{
  int index;
  
  if (argp_parse (&dbm_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

  if (argc == 0)
    {
      mu_error (_("database name not given"));
      exit (EX_USAGE);
    }
  db_name = *argv++;
  --argc;

  switch (mode)
    {
    case mode_list:
      list_database (argc, argv);
      break;
      
    case mode_create:
      if (argc)
	{
	  mu_error (_("too many arguments for create"));
	  exit (EX_USAGE);
	}
      create_database ();
      break;
      
    case mode_delete:
      delete_database (argc, argv);
      break;
      
    case mode_add:
      if (argc)
	{
	  mu_error (_("too many arguments for add"));
	  exit (EX_USAGE);
	}
      add_records (MU_STREAM_RDWR, 0);
      break;
      
    case mode_replace:
      if (argc)
	{
	  mu_error (_("too many arguments for replace"));
	  exit (EX_USAGE);
	}
      add_records (MU_STREAM_RDWR, 1);
      break;
    }
  return 0;
}

/*
  MU Setup: dbm
  mu-handler: mutool_dbm
  mu-docstring: dbm_docstring
  End MU Setup:
*/
  

