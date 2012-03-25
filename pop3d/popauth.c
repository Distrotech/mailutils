/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2012 Free Software Foundation,
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

#include "pop3d.h"
#include "mailutils/libargp.h"

int db_list (char *input_name, char *output_name);
int db_make (char *input_name, char *output_name);

#define ACT_CREATE  0
#define ACT_ADD     1
#define ACT_DELETE  2
#define ACT_LIST    3
#define ACT_CHPASS  4

static int permissions = 0600;
static int compatibility_option = 0;

struct action_data {
  int action;
  char *input_name;
  char *output_name;
  char *username;
  char *passwd;
};

void check_action(int action);
int action_create (struct action_data *ap);
int action_add (struct action_data *ap);
int action_delete (struct action_data *ap);
int action_list (struct action_data *ap);
int action_chpass (struct action_data *ap);

int (*ftab[]) (struct action_data *) = {
  action_create,
  action_add,
  action_delete,
  action_list,
  action_chpass
};

static char doc[] = N_("GNU popauth -- manage pop3 authentication database");
static error_t popauth_parse_opt  (int key, char *arg,
				   struct argp_state *astate);

void popauth_version (FILE *stream, struct argp_state *state);

#define COMPATIBILITY_OPTION 256

static struct argp_option options[] = 
{
  { NULL, 0, NULL, 0, N_("Actions are:"), 1 },
  { "add", 'a', 0, 0, N_("add user"), 1 },
  { "modify", 'm', 0, 0, N_("modify user's record (change password)"), 1 },
  { "delete", 'd', 0, 0, N_("delete user's record"), 1 },
  { "list", 'l', 0, 0, N_("list the contents of DBM file"), 1 },
  { "create", 'c', 0, 0, N_("create the DBM from a plaintext file"), 1 },

  { NULL, 0, NULL, 0,
    N_("Default action is:\n"
    "  For root: --list\n"
    "  For a user: --modify --user <username>\n"), 2 },
  
  { NULL, 0, NULL, 0, N_("Options are:"), 3 },
  { "file", 'f', N_("FILE"), 0, N_("read input from FILE (default stdin)"), 3 },
  { "output", 'o', N_("FILE"), 0, N_("direct output to file"), 3 },
  { "password", 'p', N_("STRING"), 0, N_("specify user's password"), 3 },
  { "user", 'u', N_("USERNAME"), 0, N_("specify user name"), 3 },
  { "permissions", 'P', N_("PERM"), 0, N_("force given permissions on the database"), 3 },
  { "compatibility", COMPATIBILITY_OPTION, NULL, 0,
    N_("backward compatibility mode") },
  { NULL, }
};

static struct argp argp = {
  options,
  popauth_parse_opt,
  NULL,
  doc,
  NULL,
  NULL, NULL
};

static const char *popauth_argp_capa[] = {
  "common",
  NULL
};

static void
set_db_perms (struct argp_state *astate, char *opt, int *pperm)
{
  int perm = 0;
   
  if (mu_isdigit (opt[0]))
    {
      char *p;
      perm = strtoul (opt, &p, 8);
      if (*p)
	{
	  argp_error (astate, _("invalid octal number: %s"), opt);
	  exit (EX_USAGE);
	}
    }
  *pperm = perm;
}

static error_t
popauth_parse_opt (int key, char *arg, struct argp_state *astate)
{
  struct action_data *ap = astate->input;
  switch (key)
    {
    case ARGP_KEY_INIT:
      memset (ap, 0, sizeof(*ap));
      ap->action = -1;
      break;
      
    case 'a':
      check_action (ap->action);
      ap->action = ACT_ADD;
      break;

    case 'c':
      check_action (ap->action);
      ap->action = ACT_CREATE;
      break;
      
    case 'l':
      check_action (ap->action);
      ap->action = ACT_LIST;
      break;
	
    case 'd':
      check_action (ap->action);
      ap->action = ACT_DELETE;
      break;
	  
    case 'p':
      ap->passwd = arg;
      break;
      
    case 'm':
      check_action (ap->action);
      ap->action = ACT_CHPASS;
      break;
	
    case 'f':
      ap->input_name = arg;
      break;
	  
    case 'o':
      ap->output_name = arg;
      break;
	
    case 'u':
      ap->username = arg;
      break;
	
    case 'P':
      set_db_perms (astate, arg, &permissions);
      break;

    case COMPATIBILITY_OPTION:
      compatibility_option = 1;
      break;
	
    case ARGP_KEY_FINI:
      if (ap->action == -1)
	{
	  /* Deduce the default action */
	  if (getuid () == 0)
	    ap->action = ACT_LIST;
	  else
	    ap->action = ACT_CHPASS;
	}
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  struct action_data adata;

  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mu_argp_init (NULL, NULL);
  argp_program_version_hook = popauth_version;
  if (mu_app_init (&argp, popauth_argp_capa, NULL,
		   argc, argv, 0, NULL, &adata))
    exit (EX_USAGE);

  return (*ftab[adata.action]) (&adata);
}

void
check_action (int action)
{
  if (action != -1)
    {
      mu_error (_("You may not specify more than one `-aldp' option"));
      exit (EX_USAGE);
    }
}

mu_dbm_file_t
open_db_file (int action, struct action_data *ap, int *my_file)
{
  mu_dbm_file_t db;
  struct passwd *pw;
  uid_t uid;
  int rc;
  int flags = 0;
  char *db_name = NULL;
  int fd;
  struct stat sb;
  int safety_flags = MU_FILE_SAFETY_ALL & ~MU_FILE_SAFETY_OWNER_MISMATCH;
  
  switch (action)
    {
    case ACT_CREATE:
      flags = MU_STREAM_CREAT;
      safety_flags = MU_FILE_SAFETY_NONE;
      db_name = ap->output_name;
      break;

    case ACT_ADD:
    case ACT_DELETE:
      if (!ap->input_name)
	ap->input_name = APOP_PASSFILE;
      flags = MU_STREAM_RDWR;
      db_name = ap->input_name;
      break;
      
    case ACT_LIST:
      if (!ap->input_name)
	ap->input_name = APOP_PASSFILE;
      flags = MU_STREAM_READ;
      safety_flags = MU_FILE_SAFETY_NONE;
      db_name = ap->input_name;
      break;

    case ACT_CHPASS:
      if (!ap->input_name)
	ap->input_name = APOP_PASSFILE;
      flags = MU_STREAM_RDWR;
      db_name = ap->input_name;
      break;
      
    default:
      abort ();
    }

  uid = getuid ();

  /* Adjust safety flags */
  if (permissions & 0002)
    safety_flags &= ~MU_FILE_SAFETY_WORLD_WRITABLE;
  if (permissions & 0004)
    safety_flags &= ~MU_FILE_SAFETY_WORLD_READABLE;
  if (permissions & 0020)
    safety_flags &= ~MU_FILE_SAFETY_GROUP_WRITABLE;
  if (permissions & 0040)
    safety_flags &= ~MU_FILE_SAFETY_GROUP_READABLE;

  rc = mu_dbm_create (db_name, &db, safety_flags);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("unable to create database %s: %s"),
		      db_name, mu_strerror (rc));
      exit (EX_SOFTWARE);
    }

  rc = mu_dbm_safety_check (db);
  if (rc && rc != ENOENT)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      _("%s fails safety check: %s"),
		      db_name, mu_strerror (rc));
      mu_dbm_destroy (&db);
      exit (EX_UNAVAILABLE);
    }
  
  rc = mu_dbm_open (db, flags, permissions);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_open",
		       db_name, rc);
      exit (EX_SOFTWARE);
    }

  if (uid == 0)
    return db;

  rc = mu_dbm_get_fd (db, &fd, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_dbm_get_fd",
		       db_name, rc);
      exit (EX_SOFTWARE);
    }

  if (fstat (fd, &sb))
    {
      mu_diag_funcall (MU_DIAG_ERROR, "fstat",
		       db_name, errno);
      exit (EX_SOFTWARE);
    }

  if (sb.st_uid == uid)
    {
      if (my_file)
	*my_file = 1;
      return db;
    }
  if (my_file)
    *my_file = 0;
    
  if (ap->username)
    {
      mu_error (_("Only the file owner can use --username"));
      exit (EX_USAGE);
    }

  if (action != ACT_CHPASS)
    {
      mu_error (_("Operation not allowed"));
      exit (EX_USAGE);
    }
  pw = getpwuid (uid);
  if (!pw)
    exit (EX_OSERR);
  ap->username = pw->pw_name;
  return db;
}

static void
print_entry (mu_stream_t str, struct mu_dbm_datum const *key,
	     struct mu_dbm_datum const *contents)
{
  if (compatibility_option)
    mu_stream_printf (str, "%.*s: %.*s\n",
		      (int) key->mu_dsize,
		      (char*) key->mu_dptr,
		      (int) contents->mu_dsize,
		      (char*) contents->mu_dptr);
  else
    mu_stream_printf (str, "%.*s %.*s\n",
		      (int) key->mu_dsize,
		      (char*) key->mu_dptr,
		      (int) contents->mu_dsize,
		      (char*) contents->mu_dptr);
}

int
action_list (struct action_data *ap)
{
  mu_stream_t str;
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  int rc;
  
  db = open_db_file (ACT_LIST, ap, NULL);
  
  if (ap->output_name)
    {
      int rc = mu_file_stream_create (&str, ap->output_name,
				      MU_STREAM_WRITE|MU_STREAM_CREAT);
      if (rc)
	{
	  mu_error (_("cannot create file %s: %s"),
		    ap->output_name, mu_strerror (rc));
	  return 1;
	}
      mu_stream_truncate (str, 0);
    }
  else
    {
      str = mu_strout;
      mu_stream_ref (str);
    }

  if (ap->username)
    {
      memset (&key, 0, sizeof key);
      memset (&contents, 0, sizeof contents);
      key.mu_dptr = ap->username;
      key.mu_dsize = strlen (ap->username);
      rc = mu_dbm_fetch (db, &key, &contents);
      if (rc == MU_ERR_NOENT)
	{
	  mu_error (_("no such user: %s"), ap->username);
	}
      else if (rc)
	{
	  mu_error (_("database fetch error: %s"), mu_dbm_strerror (db));
	  exit (EX_UNAVAILABLE);
	}
      else
	{
	  print_entry (str, &key, &contents);
	  mu_dbm_datum_free (&contents);
	}
    }
  else
    {
      memset (&key, 0, sizeof key);
      for (rc = mu_dbm_firstkey (db, &key); rc == 0;
	   rc = mu_dbm_nextkey (db, &key))
	{
	  memset (&contents, 0, sizeof contents);
	  rc = mu_dbm_fetch (db, &key, &contents);
	  if (rc == 0)
	    {
	      print_entry (str, &key, &contents);
	      mu_dbm_datum_free (&contents);
	    }
	  else
	    {
	      mu_error (_("database fetch error: %s"), mu_dbm_strerror (db));
	      exit (EX_UNAVAILABLE);
	    }
	  mu_dbm_datum_free (&key);
	}
    }
  
  mu_dbm_destroy (&db);
  return 0;
}

int
action_create (struct action_data *ap)
{
  int rc;
  mu_stream_t in;
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  char *buf = NULL;
  size_t size = 0, len;
  int line = 0;
  
  /* Make sure we have proper privileges if popauth is setuid */
  setuid (getuid ());
  
  if (ap->input_name)
    {
      rc = mu_file_stream_create (&in, ap->input_name, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open file %s: %s"),
		    ap->input_name, mu_strerror (rc));
	  return 1;
	}
    }
  else
    {
      ap->input_name = "";
      rc = mu_stdio_stream_create (&in, MU_STDIN_FD, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open standard input: %s"),
		    mu_strerror (rc));
	  return 1;
	}
    }
  
  if (!ap->output_name)
    ap->output_name = APOP_PASSFILE;

  db = open_db_file (ACT_CREATE, ap, NULL);

  line = 0;
  while ((rc = mu_stream_getline (in, &buf, &size, &len)) == 0
	 && len > 0)
    {
      char *str, *pass;

      line++;
      str = mu_str_stripws (buf);
      if (*str == 0 || *str == '#')
	continue;
      pass = mu_str_skip_class_comp (str, MU_CTYPE_SPACE);
      if (*pass == 0)
	{
	  mu_error (_("%s:%d: malformed line"), ap->input_name, line);
	  continue;
	}
      /* Strip trailing semicolon, when in compatibility mode. */
      if (compatibility_option && pass > str && pass[-1] == ':')
	pass[-1] = 0;
      *pass++ = 0;
      pass = mu_str_skip_class (pass, MU_CTYPE_SPACE);
      if (*pass == 0)
	{
	  mu_error (_("%s:%d: malformed line"), ap->input_name, line);
	  continue;
	}
      
      memset (&key, 0, sizeof key);
      memset (&contents, 0, sizeof contents);
      key.mu_dptr = str;
      key.mu_dsize = strlen (str);
      contents.mu_dptr = pass;
      contents.mu_dsize = strlen (pass);

      rc = mu_dbm_store (db, &key, &contents, 1);
      if (rc)
	mu_error (_("%s:%d: cannot store datum: %s"),
		  ap->input_name, line,
		  rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));
    }
  free (buf);
  mu_dbm_destroy (&db);
  mu_stream_destroy (&in);
  return 0;
}

/*FIXME
int
open_io (int action, struct action_data *ap, DBM_FILE *db, int *not_owner)
{
  int rc = check_user_perm (action, ap);
  if (not_owner)
    *not_owner = rc;
  if (mu_dbm_open (ap->input_name, db, MU_STREAM_RDWR, permissions))
    {
      mu_error (_("cannot open file %s: %s"),
		ap->input_name, mu_strerror (errno));
      return 1;
    }
  return 0;
}
*/

void
fill_pass (struct action_data *ap)
{
  if (!ap->passwd)
    {
      char *p;
      mu_stream_t in, out;
      int rc;
      
      rc = mu_stdio_stream_create (&in, MU_STDIN_FD, MU_STREAM_READ);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create",
			   "MU_STDIN_FD", rc);
	  return;
	}
 
      rc = mu_stdio_stream_create (&out, MU_STDOUT_FD, MU_STREAM_WRITE);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create",
			   "MU_STDOUT_FD", rc);
	  return;
	}

      while (1)
	{
	  if (ap->passwd)
	    free (ap->passwd);
	  rc = mu_getpass (in, out, _("Password:"), &p);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_getpass", NULL, rc);
	      exit (EX_DATAERR);
	    }
	  
	  if (!p)
	    exit (EX_DATAERR);
	  
	  ap->passwd = mu_strdup (p);
	  /* TRANSLATORS: Please try to format this string so that it has
	     the same length as the translation of 'Password:' above */
	  rc = mu_getpass (in, out, _("Confirm :"), &p);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_getpass", NULL, rc);
	      exit (EX_DATAERR);
	    }
	  
	  if (!p)
	    exit (EX_DATAERR);
	  if (strcmp (ap->passwd, p) == 0)
	    break;
	  mu_error (_("Passwords differ. Please retry."));
	}
      mu_stream_destroy (&in);
      mu_stream_destroy (&out);
    }
}

int
action_add (struct action_data *ap)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  int rc;
  
  if (!ap->username)
    {
      mu_error (_("missing username to add"));
      return 1;
    }

  db = open_db_file (ACT_ADD, ap, NULL);

  fill_pass (ap);
  
  memset (&key, 0, sizeof key);
  memset (&contents, 0, sizeof contents);
  key.mu_dptr = ap->username;
  key.mu_dsize = strlen (ap->username);
  contents.mu_dptr = ap->passwd;
  contents.mu_dsize = strlen (ap->passwd);

  rc = mu_dbm_store (db, &key, &contents, 1);
  if (rc)
    mu_error (_("cannot store datum: %s"),
	      rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));

  mu_dbm_destroy (&db);
  return rc;
}

int
action_delete (struct action_data *ap)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key;
  int rc;
  
  if (!ap->username)
    {
      mu_error (_("missing username to delete"));
      return 1;
    }

  db = open_db_file (ACT_DELETE, ap, NULL);

  memset (&key, 0, sizeof key);
  key.mu_dptr = ap->username;
  key.mu_dsize = strlen (ap->username);

  rc = mu_dbm_delete (db, &key);
  if (rc)
    mu_error (_("cannot remove record for %s: %s"),
	      ap->username,
	      rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));

  mu_dbm_destroy (&db);
  return rc;
}

int
action_chpass (struct action_data *ap)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  int rc;
  int my_file;
  
  db = open_db_file (ACT_CHPASS, ap, &my_file);

  if (!ap->username)
    {
      struct passwd *pw = getpwuid (getuid ());
      ap->username = pw->pw_name;
      printf ("Changing password for %s\n", ap->username);
    }

  memset (&key, 0, sizeof key);
  memset (&contents, 0, sizeof contents);

  key.mu_dptr = ap->username;
  key.mu_dsize = strlen (ap->username);
  rc = mu_dbm_fetch (db, &key, &contents);
  if (rc == MU_ERR_NOENT)
    {
      mu_error (_("no such user: %s"), ap->username);
      return 1;
    }
  else if (rc)
    {
      mu_error (_("database fetch error: %s"), mu_dbm_strerror (db));
      exit (EX_UNAVAILABLE);
    }
      
  if (!my_file)
    {
      char *oldpass, *p;
      
      oldpass = mu_alloc (contents.mu_dsize + 1);
      memcpy (oldpass, contents.mu_dptr, contents.mu_dsize);
      oldpass[contents.mu_dsize] = 0;
      p = getpass (_("Old Password:"));
      if (!p)
	return 1;
      if (strcmp (oldpass, p))
	{
	  mu_error (_("Sorry"));
	  return 1;
	}
    }

  fill_pass (ap);
  
  mu_dbm_datum_free (&contents);
  contents.mu_dptr = ap->passwd;
  contents.mu_dsize = strlen (ap->passwd);
  rc = mu_dbm_store (db, &key, &contents, 1);
  if (rc)
    mu_error (_("cannot replace datum: %s"),
	      rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));

  mu_dbm_destroy (&db);
  return rc;
}

void
popauth_version (FILE *stream, struct argp_state *state)
{
  mu_iterator_t itr;
  int rc;

  mu_program_version_hook (stream, state);
  fprintf (stream, _("Database formats: "));

  rc = mu_dbm_impl_iterator (&itr);
  if (rc)
    {
      fprintf (stream, "%s\n", _("unknown"));
      mu_error ("%s", mu_strerror (rc));
    }
  else
    {
      int i;
      for (mu_iterator_first (itr), i = 0; !mu_iterator_is_done (itr);
	   mu_iterator_next (itr), i++)
	{
	  struct mu_dbm_impl *impl;

	  mu_iterator_current (itr, (void**)&impl);
	  if (i)
	    fprintf (stream, ", ");
	  fprintf (stream, "%s", impl->_dbm_name);
	}
      fputc ('\n', stream);
      mu_iterator_destroy (&itr);
    }
  fprintf (stream, _("Default database location: %s\n"), APOP_PASSFILE);
  exit (EX_OK);
}
