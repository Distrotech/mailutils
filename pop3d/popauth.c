/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007-2012, 2014-2016 Free Software
   Foundation, Inc.

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
#include "mailutils/opt.h"

int db_list (char *input_name, char *output_name);
int db_make (char *input_name, char *output_name);

#define ACT_DEFAULT -1
#define ACT_CREATE  0
#define ACT_ADD     1
#define ACT_DELETE  2
#define ACT_LIST    3
#define ACT_CHPASS  4

static int permissions = 0600;
static int compatibility_option = 0;
static int action = ACT_DEFAULT;
static char *input_name;
static char *output_name;
static char *user_name;
static char *user_password;

int action_create (void);
int action_add (void);
int action_delete (void);
int action_list (void);
int action_chpass (void);

int (*ftab[]) (void) = {
  action_create,
  action_add,
  action_delete,
  action_list,
  action_chpass
};

static void
set_action (struct mu_parseopt *po, struct mu_option *opt, char const *arg)
{
  if (action != -1)
    {
      mu_parseopt_error (po,
			 _("You may not specify more than one `-aldp' option"));
      exit (po->po_exit_error);
    }
  
  switch (opt->opt_short)
    {
    case 'a':
      action = ACT_ADD;
      break;

    case 'c':
      action = ACT_CREATE;
      break;
      
    case 'l':
      action = ACT_LIST;
      break;
	
    case 'd':
      action = ACT_DELETE;
      break;

    case 'm':
      action = ACT_CHPASS;
      break;

    default:
      abort ();
    }
}      

static void
set_permissions (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  if (mu_isdigit (arg[0]))
    {
      char *p;
      permissions = strtoul (arg, &p, 8);
      if (*p == 0)
	return;
    }

  mu_parseopt_error (po, _("invalid octal number: %s"), arg);
  exit (EX_USAGE);
}

static struct mu_option popauth_options[] = {
  MU_OPTION_GROUP (N_("Actions are:")),
  { "add", 'a', NULL, MU_OPTION_DEFAULT,
    N_("add user"),
    mu_c_string, NULL, set_action },
  { "modify", 'm', NULL, MU_OPTION_DEFAULT,
    N_("modify user's record (change password)"),
    mu_c_string, NULL, set_action },
  { "delete", 'd', NULL, MU_OPTION_DEFAULT,
    N_("delete user's record"),
    mu_c_string, NULL, set_action },
  { "list", 'l', NULL, MU_OPTION_DEFAULT,
    N_("list the contents of DBM file"),
    mu_c_string, NULL, set_action },
  { "create", 'c', NULL, MU_OPTION_DEFAULT,
    N_("create the DBM from a plaintext file"),
    mu_c_string, NULL, set_action },

  MU_OPTION_GROUP (N_("Options are:")),
  { "file", 'f', N_("FILE"), MU_OPTION_DEFAULT,
    N_("read input from FILE (default stdin)"),
    mu_c_string, &input_name },
  { "output", 'o', N_("FILE"), MU_OPTION_DEFAULT,
    N_("direct output to file"),
    mu_c_string, &output_name },
  { "password", 'p', N_("STRING"), MU_OPTION_DEFAULT,
    N_("specify user's password"),
    mu_c_string, &user_password },
  { "user", 'u', N_("USERNAME"), MU_OPTION_DEFAULT,
    N_("specify user name"),
    mu_c_string, &user_name },
  { "permissions", 'P', N_("PERM"), MU_OPTION_DEFAULT,
    N_("force given permissions on the database"),
    mu_c_string, NULL, set_permissions },
  { "compatibility", 0, NULL, MU_OPTION_DEFAULT,
    N_("compatibility mode"),
    mu_c_bool, &compatibility_option },
  MU_OPTION_END
}, *options[] = { popauth_options, NULL };

void
popauth_version (struct mu_parseopt *po, mu_stream_t stream)
{
  mu_iterator_t itr;
  int rc;

  mu_version_func (po, stream);
  mu_stream_printf (stream, _("Database formats: "));

  rc = mu_dbm_impl_iterator (&itr);
  if (rc)
    {
      mu_stream_printf (stream, "%s\n", _("unknown"));
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
	    mu_stream_printf (stream, ", ");
	  mu_stream_printf (stream, "%s", impl->_dbm_name);
	}
      mu_stream_printf (stream, "\n");
      mu_iterator_destroy (&itr);
    }
  mu_stream_printf (stream, _("Default database location: %s\n"),
		    APOP_PASSFILE);
  exit (EX_OK);
}

void
popauth_help_hook (struct mu_parseopt *po, mu_stream_t stream)
{
  unsigned margin = 2;
  
  mu_stream_printf (stream, "%s", _("Default action is:\n"));
  mu_stream_ioctl (stream, MU_IOCTL_WORDWRAPSTREAM,
		   MU_IOCTL_WORDWRAP_SET_MARGIN, &margin);
  mu_stream_printf (stream, "%s\n", _("For root: --list"));
  mu_stream_printf (stream, "%s\n",
		    _("For a user: --modify --user <username>"));
}

int
main (int argc, char **argv)
{
  struct mu_parseopt po;
  int flags;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  mu_set_program_name (argv[0]);

  po.po_prog_doc = N_("GNU popauth -- manage pop3 authentication database");
  po.po_package_name = PACKAGE_NAME;
  po.po_package_url = PACKAGE_URL;
  po.po_bug_address = PACKAGE_BUGREPORT;
  po.po_extra_info =
    N_("General help using GNU software: <http://www.gnu.org/gethelp/>");
  po.po_version_hook = popauth_version;
  po.po_help_hook = popauth_help_hook;
  
  flags = MU_PARSEOPT_IMMEDIATE
    | MU_PARSEOPT_DATA
    | MU_PARSEOPT_PROG_DOC
    | MU_PARSEOPT_PACKAGE_NAME
    | MU_PARSEOPT_PACKAGE_URL
    | MU_PARSEOPT_BUG_ADDRESS
    | MU_PARSEOPT_EXTRA_INFO
    | MU_PARSEOPT_VERSION_HOOK
    | MU_PARSEOPT_HELP_HOOK;

  if (mu_parseopt (&po, argc, argv, options, flags))
    exit (po.po_exit_error);

  if (argc > po.po_arg_start)
    {
      mu_parseopt_error (&po, _("too many arguments"));
      exit (po.po_exit_error);
    }
  mu_parseopt_free (&po);

  if (action == ACT_DEFAULT)
    {
      if (getuid () == 0)
	action = ACT_LIST;
      else
	action = ACT_CHPASS;
    }
  return (*ftab[action]) ();
}

mu_dbm_file_t
open_db_file (int action, int *my_file)
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
      db_name = output_name;
      break;

    case ACT_ADD:
    case ACT_DELETE:
      if (!input_name)
	input_name = APOP_PASSFILE;
      flags = MU_STREAM_RDWR;
      db_name = input_name;
      break;
      
    case ACT_LIST:
      if (!input_name)
	input_name = APOP_PASSFILE;
      flags = MU_STREAM_READ;
      safety_flags = MU_FILE_SAFETY_NONE;
      db_name = input_name;
      break;

    case ACT_CHPASS:
      if (!input_name)
	input_name = APOP_PASSFILE;
      flags = MU_STREAM_RDWR;
      db_name = input_name;
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
    
  if (user_name)
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
  user_name = pw->pw_name;
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
action_list (void)
{
  mu_stream_t str;
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  int rc;
  
  db = open_db_file (ACT_LIST, NULL);
  
  if (output_name)
    {
      int rc = mu_file_stream_create (&str, output_name,
				      MU_STREAM_WRITE|MU_STREAM_CREAT);
      if (rc)
	{
	  mu_error (_("cannot create file %s: %s"),
		    output_name, mu_strerror (rc));
	  return 1;
	}
      mu_stream_truncate (str, 0);
    }
  else
    {
      str = mu_strout;
      mu_stream_ref (str);
    }

  if (user_name)
    {
      memset (&key, 0, sizeof key);
      memset (&contents, 0, sizeof contents);
      key.mu_dptr = user_name;
      key.mu_dsize = strlen (user_name);
      rc = mu_dbm_fetch (db, &key, &contents);
      if (rc == MU_ERR_NOENT)
	{
	  mu_error (_("no such user: %s"), user_name);
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
action_create (void)
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
  
  if (input_name)
    {
      rc = mu_file_stream_create (&in, input_name, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open file %s: %s"),
		    input_name, mu_strerror (rc));
	  return 1;
	}
    }
  else
    {
      input_name = "";
      rc = mu_stdio_stream_create (&in, MU_STDIN_FD, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open standard input: %s"),
		    mu_strerror (rc));
	  return 1;
	}
    }
  
  if (!output_name)
    output_name = APOP_PASSFILE;

  db = open_db_file (ACT_CREATE, NULL);

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
	  mu_error (_("%s:%d: malformed line"), input_name, line);
	  continue;
	}
      /* Strip trailing semicolon, when in compatibility mode. */
      if (compatibility_option && pass > str && pass[-1] == ':')
	pass[-1] = 0;
      *pass++ = 0;
      pass = mu_str_skip_class (pass, MU_CTYPE_SPACE);
      if (*pass == 0)
	{
	  mu_error (_("%s:%d: malformed line"), input_name, line);
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
		  input_name, line,
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
  if (mu_dbm_open (input_name, db, MU_STREAM_RDWR, permissions))
    {
      mu_error (_("cannot open file %s: %s"),
		input_name, mu_strerror (errno));
      return 1;
    }
  return 0;
}
*/

void
fill_pass (void)
{
  if (!user_password)
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
	  if (user_password)
	    free (user_password);
	  rc = mu_getpass (in, out, _("Password:"), &p);
	  if (rc)
	    {
	      mu_diag_funcall (MU_DIAG_ERROR, "mu_getpass", NULL, rc);
	      exit (EX_DATAERR);
	    }
	  
	  if (!p)
	    exit (EX_DATAERR);
	  
	  user_password = mu_strdup (p);
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
	  if (strcmp (user_password, p) == 0)
	    break;
	  mu_error (_("Passwords differ. Please retry."));
	}
      mu_stream_destroy (&in);
      mu_stream_destroy (&out);
    }
}

int
action_add (void)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  int rc;
  
  if (!user_name)
    {
      mu_error (_("missing user name to add"));
      return 1;
    }

  db = open_db_file (ACT_ADD, NULL);

  fill_pass ();
  
  memset (&key, 0, sizeof key);
  memset (&contents, 0, sizeof contents);
  key.mu_dptr = user_name;
  key.mu_dsize = strlen (user_name);
  contents.mu_dptr = user_password;
  contents.mu_dsize = strlen (user_password);

  rc = mu_dbm_store (db, &key, &contents, 1);
  if (rc)
    mu_error (_("cannot store datum: %s"),
	      rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));

  mu_dbm_destroy (&db);
  return rc;
}

int
action_delete (void)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key;
  int rc;
  
  if (!user_name)
    {
      mu_error (_("missing username to delete"));
      return 1;
    }

  db = open_db_file (ACT_DELETE, NULL);

  memset (&key, 0, sizeof key);
  key.mu_dptr = user_name;
  key.mu_dsize = strlen (user_name);

  rc = mu_dbm_delete (db, &key);
  if (rc)
    mu_error (_("cannot remove record for %s: %s"),
	      user_name,
	      rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));

  mu_dbm_destroy (&db);
  return rc;
}

int
action_chpass (void)
{
  mu_dbm_file_t db;
  struct mu_dbm_datum key, contents;
  int rc;
  int my_file;
  
  db = open_db_file (ACT_CHPASS, &my_file);

  if (!user_name)
    {
      struct passwd *pw = getpwuid (getuid ());
      user_name = pw->pw_name;
      printf ("Changing password for %s\n", user_name);
    }

  memset (&key, 0, sizeof key);
  memset (&contents, 0, sizeof contents);

  key.mu_dptr = user_name;
  key.mu_dsize = strlen (user_name);
  rc = mu_dbm_fetch (db, &key, &contents);
  if (rc == MU_ERR_NOENT)
    {
      mu_error (_("no such user: %s"), user_name);
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

  fill_pass ();
  
  mu_dbm_datum_free (&contents);
  contents.mu_dptr = user_password;
  contents.mu_dsize = strlen (user_password);
  rc = mu_dbm_store (db, &key, &contents, 1);
  if (rc)
    mu_error (_("cannot replace datum: %s"),
	      rc == MU_ERR_FAILURE ?
		     mu_dbm_strerror (db) : mu_strerror (rc));

  mu_dbm_destroy (&db);
  return rc;
}

