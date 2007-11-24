/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 
   2007 Free Software Foundation, Inc.

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

#include <mail.local.h>
#include "mailutils/libargp.h"

int multiple_delivery;     /* Don't return errors when delivering to multiple
			      recipients */
int ex_quota_tempfail;     /* Return temporary failure if mailbox quota is
			      exceeded. If this variable is not set, mail.local
			      will return "service unavailable" */
int exit_code = EX_OK;     /* Exit code to be used */
uid_t uid;                 /* Current user id */
char *quotadbname = NULL;  /* Name of mailbox quota database */
char *quota_query = NULL;  /* SQL query to retrieve mailbox quota */

/* Debuggig options */
int debug_level;           /* General debugging level */ 
int debug_flags;           /* Mailutils debugging flags */
int sieve_debug_flags;     /* Sieve debugging flags */
int sieve_enable_log;      /* Enables logging of executed Sieve actions */
char *message_id_header;   /* Use the value of this header as message
			      identifier when logging Sieve actions */
mu_debug_t mudebug;        /* Mailutils debugging object */


#define MAXFD 64
#define EX_QUOTA() (ex_quota_tempfail ? EX_TEMPFAIL : EX_UNAVAILABLE)

void close_fds (void);
int make_tmp (const char *from, mu_mailbox_t *mbx);
void deliver (mu_mailbox_t msg, char *name);
void guess_retval (int ec);
void notify_biff (mu_mailbox_t mbox, char *name, size_t size);

const char *program_version = "mail.local (" PACKAGE_STRING ")";
static char doc[] =
N_("GNU mail.local -- the local MDA")
"\v"
N_("Debug flags are:\n\
  g - guimb stack traces\n\
  T - mailutils traces (MU_DEBUG_TRACE)\n\
  P - network protocols (MU_DEBUG_PROT)\n\
  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n\
  i - sieve instructions trace (MU_SIEVE_DEBUG_INSTR)\n\
  l - sieve action logs\n\
  0-9 - Set mail.local debugging level\n");

static char args_doc[] = N_("recipient [recipient ...]");

#define ARG_MULTIPLE_DELIVERY 1
#define ARG_QUOTA_TEMPFAIL    2
#define ARG_MESSAGE_ID_HEADER 3
#define ARG_QUOTA_QUERY       4

static struct argp_option options[] = 
{
  { "ex-multiple-delivery-success", ARG_MULTIPLE_DELIVERY, NULL, 0,
    N_("Do not return errors when delivering to multiple recipients"), 0 },
  { "ex-quota-tempfail", ARG_QUOTA_TEMPFAIL, NULL, 0,
    N_("Return temporary failure if disk or mailbox quota is exceeded"), 0 },
  { "from", 'f', N_("EMAIL"), 0,
    N_("Specify the sender's name") },
  { NULL, 'r', NULL, OPTION_ALIAS, NULL },
#ifdef USE_DBM
  { "quota-db", 'q', N_("FILE"), 0,
    N_("Specify path to quota DBM database"), 0 },
#endif
#ifdef USE_SQL
  { "quota-query", ARG_QUOTA_QUERY, N_("STRING"), 0,
    N_("SQL query to retrieve mailbox quota"), 0 },
#endif
  { "sieve", 'S', N_("PATTERN"), 0,
    N_("Set name pattern for user-defined Sieve mail filters"), 0 },
  { "message-id-header", ARG_MESSAGE_ID_HEADER, N_("STRING"), 0,
    N_("Identify messages by the value of this header when logging Sieve actions"), 0 },
#ifdef WITH_GUILE
  { "source", 's', N_("PATTERN"), 0,
    N_("Set name pattern for user-defined Scheme mail filters"), 0 },
#endif
  { "debug", 'x', N_("FLAGS"), 0,
    N_("Enable debugging"), 0 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);

static struct argp argp = {
  options,
  parse_opt,
  args_doc, 
  doc,
  NULL,
  NULL, NULL
};

static const char *argp_capa[] = {
  "auth",
  "common",
  "license",
  "logging",
  "mailbox",
  "mailer",
  NULL
};

char *from = NULL;
char *progfile_pattern = NULL;
char *sieve_pattern = NULL;
char *saved_envelope;  /* A hack to spare mu_envelope_ calls */

#define D_DEFAULT "9s"

static void
set_debug_flags (const mu_cfg_locus_t *locus, const char *arg)
{
  for (; *arg; arg++)
    {
      switch (*arg)
	{
	case 'g':
#ifdef WITH_GUILE
	  debug_guile = 1;
#endif
	  break;

	case 't':
	  sieve_debug_flags |= MU_SIEVE_DEBUG_TRACE;
	  break;
	  
	case 'i':
	  sieve_debug_flags |= MU_SIEVE_DEBUG_INSTR;
	  break;
	  
	case 'l':
	  sieve_enable_log = 1;
	  break;
	  
	case 'T':
	  debug_flags |= MU_DEBUG_TRACE;
	  break;
	  
	case 'P':
	  debug_flags |= MU_DEBUG_PROT;
	  break;

	default:
	  if (isdigit (*arg))
	    debug_level = *arg - '0';
	  else if (locus)
	    mu_error (_("%s:%d: %c is not a valid debug flag"),
		      locus->file, locus->line, *arg);
	  else
	    mu_error (_("%c is not a valid debug flag"), *arg);
	  break;
	}
    }
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_MULTIPLE_DELIVERY:
      multiple_delivery = 1;
      break;

    case ARG_QUOTA_TEMPFAIL:
      ex_quota_tempfail = 1;
      break;

    case ARG_MESSAGE_ID_HEADER:
      message_id_header = arg;
      break;

    case ARG_QUOTA_QUERY:
      quota_query = arg;
      break;
      
    case 'r':
    case 'f':
      if (from != NULL)
	{
	  argp_error (state, _("Multiple --from options"));
	  return EX_USAGE;
	}
      from = arg;
      break;
      
#ifdef USE_DBM
    case 'q':
      quotadbname = arg;
      break;
#endif

#ifdef WITH_GUILE	
    case 's':
      progfile_pattern = arg;
      break;
#endif

    case 'S':
      sieve_pattern = arg;
      break;
      
    case 'x':
      if (!arg)
	arg = D_DEFAULT;
      set_debug_flags (NULL, arg);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_ERROR:
      exit (EX_USAGE);
    }
  return 0;
}


static int
cb_debug (mu_cfg_locus_t *locus, void *data, char *arg)
{
  set_debug_flags (locus, arg);
  return 0;
}

struct mu_cfg_param mail_local_cfg_param[] = {
  { "ex-multiple-delivery-success", mu_cfg_bool, &multiple_delivery },
  { "ex-quota-tempfail", mu_cfg_bool, &ex_quota_tempfail },
  { "from", mu_cfg_string, &from },
#ifdef USE_DBM
  { "quota-db", mu_cfg_string, &quotadbname },
#endif
#ifdef USE_SQL
  { "quota-query", mu_cfg_string, &quota_query },
#endif
  { "sieve", mu_cfg_string, &sieve_pattern },
  { "message-id-header", mu_cfg_string, &message_id_header },
#ifdef WITH_GUILE
  { "source", mu_cfg_string, &progfile_pattern },
#endif
  { "debug", mu_cfg_callback, NULL, cb_debug },
  { NULL }
};



static int
_sieve_debug_printer (void *unused, const char *fmt, va_list ap)
{
  vsyslog (LOG_DEBUG, fmt, ap);
  return 0;
}

static void
_sieve_action_log (void *user_name,
		   const mu_sieve_locus_t *locus, size_t msgno,
		   mu_message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  char *text = NULL;
  
  if (message_id_header)
    {
      mu_header_t hdr = NULL;
      char *val = NULL;
      mu_message_get_header (msg, &hdr);
      if (mu_header_aget_value (hdr, message_id_header, &val) == 0
	  || mu_header_aget_value (hdr, MU_HEADER_MESSAGE_ID, &val) == 0)
	{
	  asprintf (&text, _("%s:%lu: %s on msg %s"),
		    locus->source_file,
		    (unsigned long) locus->source_line,
		    action, val);
	  free (val);
	}
    }
  if (text == NULL)
    {
      size_t uid = 0;
      mu_message_get_uid (msg, &uid);
      asprintf (&text, _("%s:%lu: %s on msg uid %d"),
		locus->source_file,
		(unsigned long)	locus->source_line,
		action, uid);
    }
  
  if (fmt && strlen (fmt))
    {
      char *diag = NULL;
      vasprintf (&diag, fmt, ap);
      syslog (LOG_NOTICE, _("(user %s) %s: %s"),
	      (char*) user_name, text, diag);
      free (diag);
    }
  else
    syslog (LOG_NOTICE, _("(user %s) %s"), (char*) user_name, text);
  free (text);
}

static int
_sieve_parse_error (void *user_name, const char *filename, int lineno,
		    const char *fmt, va_list ap)
{
  char *text;
  vasprintf (&text, fmt, ap);
  if (filename)
    {
      char *loc;
      asprintf (&loc, "%s:%d: ", filename, lineno);
      syslog (LOG_ERR, "%s: %s", loc, text);
      free (loc);
    }
  else
    syslog (LOG_ERR, _("(user %s) %s"), (char*)user_name, text);
  free (text);
  return 0;
}

int
main (int argc, char *argv[])
{
  mu_mailbox_t mbox = NULL;
  int arg_index;
  
  /* Preparative work: close inherited fds, force a reasonable umask
     and prepare a logging. */
  close_fds ();
  umask (0077);

  /* Native Language Support */
  mu_init_nls ();

  /* Default locker settings */
  mu_locker_set_default_flags (MU_LOCKER_PID|MU_LOCKER_RETRY,
			       mu_locker_assign);
  mu_locker_set_default_retry_timeout (1);
  mu_locker_set_default_retry_count (300);

  /* Register needed modules */
  MU_AUTH_REGISTER_ALL_MODULES();
  /* Parse command line */
  mu_gocs_register ("sieve", mu_sieve_module_init);
  mu_argp_init (program_version, NULL);
  if (mu_app_init (&argp, argp_capa, mail_local_cfg_param,
		   argc, argv, 0, &arg_index, NULL))
    exit (EX_CONFIG);

  uid = getuid ();

  if (uid == 0)
    {
      openlog ("mail.local", LOG_PID, log_facility);
      mu_error_set_print (mu_syslog_error_printer);
    }
  
  if (debug_flags)
    {
      int rc;
      
      if ((rc = mu_debug_create (&mudebug, NULL)))
	{
	  mu_error (_("mu_debug_create failed: %s\n"), mu_strerror (rc));
	  exit (EX_TEMPFAIL);
	}
      if ((rc = mu_debug_set_level (mudebug, debug_flags)))
	{
	  mu_error (_("mu_debug_set_level failed: %s\n"),
		    mu_strerror (rc));
	  exit (EX_TEMPFAIL);
	}
    }
  
  argc -= arg_index;
  argv += arg_index;

  if (!argc)
    {
      if (uid)
	{
	  static char *s_argv[2];
	  struct mu_auth_data *auth = mu_get_auth_by_uid (uid);

	  if (!uid)
	    {
	      mu_error (_("Cannot get username"));
	      return EX_UNAVAILABLE;
	    }
	    
	  s_argv[0] = auth->name;
	  argv = s_argv;
	  argc = 1;
	}
      else
	{
	  mu_error (_("Missing arguments. Try --help for more info."));
	  return EX_USAGE;
	}
    }

  mu_register_all_formats ();
  /* Possible supported mailers.  */
  mu_registrar_record (mu_sendmail_record);
  mu_registrar_record (mu_smtp_record);

  if (make_tmp (from, &mbox))
    exit (exit_code);
  
  if (multiple_delivery)
    multiple_delivery = argc > 1;

#ifdef WITH_GUILE
  if (progfile_pattern)
    {
      struct mda_data mda_data;
      
      memset (&mda_data, 0, sizeof mda_data);
      mda_data.mbox = mbox;
      mda_data.argv = argv;
      mda_data.progfile_pattern = progfile_pattern;
      return prog_mda (&mda_data);
    }
#endif
  
  for (; *argv; argv++)
    mda (mbox, *argv);
  return exit_code;
}

int
sieve_test (struct mu_auth_data *auth, mu_mailbox_t mbx)
{
  int rc = 1;
  char *progfile;
    
  if (!sieve_pattern)
    return 1;

  progfile = mu_expand_path_pattern (sieve_pattern, auth->name);
  if (access (progfile, R_OK))
    {
      if (debug_level > 2)
	syslog (LOG_DEBUG, _("Access to %s failed: %m"), progfile);
    }
  else
    {
      mu_sieve_machine_t mach;
      rc = mu_sieve_machine_init (&mach, auth->name);
      if (rc)
	{
	  mu_error (_("Cannot initialize sieve machine: %s"),
		    mu_strerror (rc));
	}
      else
	{
	  mu_sieve_set_debug (mach, _sieve_debug_printer);
	  mu_sieve_set_debug_level (mach, mudebug, sieve_debug_flags);
	  mu_sieve_set_parse_error (mach, _sieve_parse_error);
	  if (sieve_enable_log)
	    mu_sieve_set_logger (mach, _sieve_action_log);
	  
	  rc = mu_sieve_compile (mach, progfile);
	  if (rc == 0)
	    {
	      mu_attribute_t attr;
	      mu_message_t msg = NULL;
		
	      mu_mailbox_get_message (mbx, 1, &msg);
	      mu_message_get_attribute (msg, &attr);
	      mu_attribute_unset_deleted (attr);
	      if (switch_user_id (auth, 1) == 0)
		{
		  chdir (auth->dir);
		
		  rc = mu_sieve_message (mach, msg);
 		  if (rc == 0)
		    rc = mu_attribute_is_deleted (attr) == 0;

		  switch_user_id (auth, 0);
		  chdir ("/");
		}
	      mu_sieve_machine_destroy (&mach);
	    }
	}
    }
  free (progfile);
  return rc;
}

int
mda (mu_mailbox_t mbx, char *username)
{
  deliver (mbx, username);

  if (multiple_delivery)
    exit_code = EX_OK;

  return exit_code;
}

void
close_fds ()
{
  int i;
  long fdlimit = MAXFD;

#if defined (HAVE_SYSCONF) && defined (_SC_OPEN_MAX)
  fdlimit = sysconf (_SC_OPEN_MAX);
#elif defined (HAVE_GETDTABLESIZE)
  fdlimit = getdtablesize ();
#endif

  for (i = 3; i < fdlimit; i++)
    close (i);
}

int
switch_user_id (struct mu_auth_data *auth, int user)
{
  int rc;
  uid_t uid;
  
  if (!auth || auth->change_uid == 0)
    return 0;
  
  if (user)
    uid = auth->uid;
  else
    uid = 0;
  
#if defined(HAVE_SETREUID)
  rc = setreuid (0, uid);
#elif defined(HAVE_SETRESUID)
  rc = setresuid (-1, uid, -1);
#elif defined(HAVE_SETEUID)
  rc = seteuid (uid);
#else
# error "No way to reset user privileges?"
#endif
  if (rc < 0)
    mailer_err ("cannot set user priviledes", auth->name,
		"setreuid(0, %d): %s (r=%d, e=%d)",
		uid, strerror (errno), getuid (), geteuid ());
  return rc;
}

static int
tmp_write (mu_stream_t stream, mu_off_t *poffset, char *buf, size_t len)
{
  size_t n = 0;
  int status = mu_stream_write (stream, buf, len, *poffset, &n);
  if (status == 0 && n != len)
    status = EIO;
  *poffset += n;
  return status;
}

int
make_tmp (const char *from, mu_mailbox_t *mbox)
{
  mu_stream_t stream;
  char *buf = NULL;
  size_t n = 0;
  mu_off_t offset = 0;
  size_t line;
  int status;
  static char *newline = "\n";
  char *tempfile;
  
  tempfile = mu_tempname (NULL);
  if ((status = mu_file_stream_create (&stream, tempfile, MU_STREAM_RDWR)))
    {
      mailer_err ("Temporary failure", NULL,
		  _("Unable to open temporary file: %s"),
		  mu_strerror (status));
      exit (exit_code);
    }

  if ((status = mu_stream_open (stream)))
    {
      mailer_err ("Temporary failure", NULL,
		  _("unable to open temporary file: %s"), mu_strerror (status));
      exit (exit_code);
    }
  
  line = 0;
  while (getline (&buf, &n, stdin) > 0)
    {
      line++;
      if (line == 1)
	{
	  if (memcmp (buf, "From ", 5))
	    {
	      struct mu_auth_data *auth = NULL;
	      if (!from)
		{
		  auth = mu_get_auth_by_uid (uid);
		  if (auth)
		    from = auth->name;
		}
	      if (from)
		{
		  time_t t;
		  
		  time (&t);
		  asprintf (&saved_envelope, "From %s %s", from, ctime (&t));
		  status = tmp_write (stream, &offset, saved_envelope,
				      strlen (saved_envelope));
		}
	      else
		{
		  mailer_err ("Cannot determine sender address", NULL,
			      _("Cannot determine sender address"));
		  exit (EX_UNAVAILABLE);
		}
	      if (auth)
		mu_auth_data_free (auth);
	    }
	  else
	    {
	      saved_envelope = strdup (buf);
	      if (!saved_envelope)
		status = ENOMEM;
	    }
	}
      else if (!memcmp (buf, "From ", 5))
	{
	  static char *escape = ">";
	  status = tmp_write (stream, &offset, escape, 1);
	}

      if (!status)
	status = tmp_write (stream, &offset, buf, strlen (buf));
      
      if (status)
	{
	  mailer_err ("Temporary failure", NULL,
		      _("Error writing temporary file: %s"),
		      mu_strerror (status));
	  mu_stream_destroy (&stream, mu_stream_get_owner (stream));
	  return status;
	}
    }
  
  if (buf && strchr (buf, '\n') == NULL)
    status = tmp_write (stream, &offset, newline, 1);

  status = tmp_write (stream, &offset, newline, 1);
  free (buf);
  unlink (tempfile);
  free (tempfile);
  
  if (status)
    {
      errno = status;
      mailer_err ("Error writing temporary file", NULL,
		  _("Error writing temporary file: %s"), mu_strerror (status));
      mu_stream_destroy (&stream, mu_stream_get_owner (stream));
      return status;
    }

  mu_stream_flush (stream);
  if ((status = mu_mailbox_create (mbox, "/dev/null")) 
      || (status = mu_mailbox_open (*mbox, MU_STREAM_READ))
      || (status = mu_mailbox_set_stream (*mbox, stream)))
    {
      mailer_err ("Error opening temporary file", NULL,
		  _("Error opening temporary file: %s"),
		  mu_strerror (status));
      mu_stream_destroy (&stream, mu_stream_get_owner (stream));
      return status;
    }

  status = mu_mailbox_messages_count (*mbox, &n);
  if (status)
    {
      errno = status;
      mailer_err ("Error collecting message", NULL,
		  _("Error creating temporary message: %s"),
		  mu_strerror (status));
      mu_stream_destroy (&stream, mu_stream_get_owner (stream));
      return status;
    }

  return status;
}

void
deliver (mu_mailbox_t imbx, char *name)
{
  mu_mailbox_t mbox;
  mu_message_t msg;
  char *path;
  mu_url_t url = NULL;
  mu_locker_t lock;
  struct mu_auth_data *auth;
  int status;
  mu_stream_t istream, ostream;
  mu_off_t size;
  int failed = 0;
  
  auth = mu_get_auth_by_name (name);
  if (!auth)
    {
      mailer_err ("no such user", name,
		  _("%s: no such user"), name);
      exit_code = EX_UNAVAILABLE;
      return;
    }
  if (uid)
    auth->change_uid = 0;
  
  if (!sieve_test (auth, imbx))
    {
      exit_code = EX_OK;
      mu_auth_data_free (auth);
      return;
    }

  if ((status = mu_mailbox_get_message (imbx, 1, &msg)) != 0)
    {
      mailer_err ("cannot read message", name,
		  _("Cannot get message from the temporary mailbox: %s"),
		  mu_strerror (status));
      mu_auth_data_free (auth);
      return;
    }

  if ((status = mu_message_get_stream (msg, &istream)) != 0)
    {
      mailer_err ("cannot read message", name,
		  _("Cannot get input message stream: %s"),
		  mu_strerror (status));
      mu_auth_data_free (auth);
      return;
    }
  
  if ((status = mu_mailbox_create (&mbox, auth->mailbox)) != 0)
    {
      mailer_err ("cannot read message", name,
		  _("Cannot open mailbox %s: %s"),
		  auth->mailbox, mu_strerror (status));
      mu_auth_data_free (auth);
      return;
    }

  mu_mailbox_get_url (mbox, &url);
  path = (char*) mu_url_to_string (url);

  /* Actually open the mailbox. Switch to the user's euid to make
     sure the maildrop file will have right privileges, in case it
     will be created */
  if (switch_user_id (auth, 1))
    return;
  status = mu_mailbox_open (mbox, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (switch_user_id (auth, 0))
    return;
  if (status != 0)
    {
      mailer_err ("cannot open mailbox", name,
		  _("Cannot open mailbox %s: %s"), path, mu_strerror (status));
      mu_mailbox_destroy (&mbox);
      return;
    }
  
  mu_mailbox_get_locker (mbox, &lock);

  status = mu_locker_lock (lock);

  if (status)
    {
      mailer_err ("cannot lock mailbox", name,
		  _("Cannot lock mailbox `%s': %s"), path,
		  mu_strerror (status));
      mu_mailbox_destroy (&mbox);
      exit_code = EX_TEMPFAIL;
      return;
    }

  if ((status = mu_mailbox_get_stream (mbox, &ostream)))
    {
      mailer_err ("cannot access mailbox", name,
		  _("Cannot get stream for mailbox %s: %s"),
		  path, mu_strerror (status));
      mu_mailbox_destroy (&mbox);
      return;
    }

  if ((status = mu_stream_size (ostream, &size)))
    {
      mailer_err ("cannot access mailbox", name,
		  _("Cannot get stream size (mailbox %s): %s"),
		  path, mu_strerror (status));
      mu_mailbox_destroy (&mbox);
      return;
    }

#if defined(USE_MAILBOX_QUOTAS)
  {
    mu_off_t n;
    mu_off_t isize;

    switch (check_quota (auth, size, &n))
      {
      case MQUOTA_EXCEEDED:
	mailer_err ("mailbox quota exceeded for this recipient", name,
		    _("%s: mailbox quota exceeded for this recipient"), name);
	exit_code = EX_QUOTA();
	failed++;
	break;
	
      case MQUOTA_UNLIMITED:
	break;
	
      default:
	if ((status = mu_stream_size (istream, &isize)))
	  {
	    mailer_err ("cannot access mailbox", name,
			_("Cannot get stream size (input message %s): %s"),
			path, mu_strerror (status));
	    exit_code = EX_UNAVAILABLE;
	    failed++;
	  }
	else if (isize > n)
	  {
	    mailer_err ("message would exceed maximum mailbox size for this recipient",
			name,
			_("%s: message would exceed maximum mailbox size for this recipient"),
			name);
	    exit_code = EX_QUOTA();
	    failed++;
	  }
	break;
      }
  }
#endif
  
  if (!failed && switch_user_id (auth, 1) == 0)
    {
      size_t nrd;
      char *buf = NULL;
      mu_off_t bufsize = 1024;

      mu_stream_size (istream, &bufsize);
      for (; (buf = malloc (bufsize)) == NULL && bufsize > 1; bufsize /= 2)
	;
      
      if (!buf)
	{
	  status = errno = ENOMEM;
	  failed++;
	}
      else if ((status = mu_stream_seek (ostream, size, SEEK_SET)) != 0
	       || (status = mu_stream_seek (istream, 0, SEEK_SET) != 0))
	/* nothing: the error will be reported later */;
      else
	{
	  status = mu_stream_sequential_write (ostream, saved_envelope,
					       strlen (saved_envelope));
	  if (status == 0)
	    {
	      while ((status = mu_stream_sequential_read (istream,
							  buf, bufsize,
							  &nrd))
		     == 0
		     && nrd > 0)
		{
		  status = mu_stream_sequential_write (ostream, buf, nrd);
		  if (status)
		    break;
		}
	  
	      free (buf);
	    }
	}
      
      switch_user_id (auth, 0);

      if (status)
	{
	  /* Undo the delivery by truncating the mailbox back to its
	     original size */
	  int rc = mu_stream_truncate (ostream, size);
	  if (rc)
	    mailer_err ("write error", name,
			_("Error writing to mailbox: %s. Mailbox NOT truncated: %s"),
			mu_strerror (status), mu_strerror (rc));
	  else  
	    mailer_err ("write error", name,
			_("Error writing to mailbox: %s"),
	  	        mu_strerror (status));
	}
    }

  if (!failed)
    notify_biff (mbox, name, size);

  mu_locker_unlock (lock);

  mu_auth_data_free (auth);
  mu_mailbox_close (mbox);
  mu_mailbox_destroy (&mbox);
}

void
notify_biff (mu_mailbox_t mbox, char *name, size_t size)
{
  static int fd = -1;
  mu_url_t url = NULL;
  char *buf = NULL;
  static struct sockaddr_in inaddr;
    
  if (fd == -1)
    {
      struct servent *sp;
      
      if ((sp = getservbyname ("biff", "udp")) == NULL)
	return;

      inaddr.sin_family = AF_INET;
      inaddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
      inaddr.sin_port = sp->s_port;
      
      fd = socket (PF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
	fd = -2; /* Mark failed initialization */
    }

  if (fd < 0)
    return;
  
  mu_mailbox_get_url (mbox, &url);
  asprintf (&buf, "%s@%lu:%s", name,
	    (unsigned long) size, mu_url_to_string (url));
  if (buf)
    {
      sendto (fd, buf, strlen (buf), 0, (struct sockaddr *)&inaddr,
	      sizeof inaddr);
      free (buf);
    }
}

void
mailer_err (char *msg, char *arg, char *fmt, ...)
{
  va_list ap;

  guess_retval (errno);
  if (arg)
    fprintf (stderr, "<%s>: %s", arg, msg);
  else
    fputs (msg, stderr);
  fputc ('\n', stderr);
  if (fmt)
    {
      va_start (ap, fmt);
      mu_verror (fmt, ap);
      va_end (ap);
    }
}

int temp_errors[] = {
#ifdef EAGAIN
  EAGAIN, /* Try again */
#endif
#ifdef EBUSY
  EBUSY, /* Device or resource busy */
#endif
#ifdef EPROCLIM
  EPROCLIM, /* Too many processes */
#endif
#ifdef EUSERS
  EUSERS, /* Too many users */
#endif
#ifdef ECONNABORTED
  ECONNABORTED, /* Software caused connection abort */
#endif
#ifdef ECONNREFUSED
  ECONNREFUSED, /* Connection refused */
#endif
#ifdef ECONNRESET
  ECONNRESET, /* Connection reset by peer */
#endif
#ifdef EDEADLK
  EDEADLK, /* Resource deadlock would occur */
#endif
#ifdef EDEADLOCK
  EDEADLOCK, /* Resource deadlock would occur */
#endif
#ifdef EFBIG
  EFBIG, /* File too large */
#endif
#ifdef EHOSTDOWN
  EHOSTDOWN, /* Host is down */
#endif
#ifdef EHOSTUNREACH
  EHOSTUNREACH, /* No route to host */
#endif
#ifdef EMFILE
  EMFILE, /* Too many open files */
#endif
#ifdef ENETDOWN
  ENETDOWN, /* Network is down */
#endif
#ifdef ENETUNREACH
  ENETUNREACH, /* Network is unreachable */
#endif
#ifdef ENETRESET
  ENETRESET, /* Network dropped connection because of reset */
#endif
#ifdef ENFILE
  ENFILE, /* File table overflow */
#endif
#ifdef ENOBUFS
  ENOBUFS, /* No buffer space available */
#endif
#ifdef ENOMEM
  ENOMEM, /* Out of memory */
#endif
#ifdef ENOSPC
  ENOSPC, /* No space left on device */
#endif
#ifdef EROFS
  EROFS, /* Read-only file system */
#endif
#ifdef ESTALE
  ESTALE, /* Stale NFS file handle */
#endif
#ifdef ETIMEDOUT
  ETIMEDOUT,  /* Connection timed out */
#endif
#ifdef EWOULDBLOCK
  EWOULDBLOCK, /* Operation would block */
#endif
};
  

void
guess_retval (int ec)
{
  int i;
  /* Temporary failures override hard errors. */
  if (exit_code == EX_TEMPFAIL)
    return;
#ifdef EDQUOT
  if (ec == EDQUOT)
    {
      exit_code = EX_QUOTA();
      return;
    }
#endif

  for (i = 0; i < sizeof (temp_errors)/sizeof (temp_errors[0]); i++)
    if (temp_errors[i] == ec)
      {
	exit_code = EX_TEMPFAIL;
	return;
      }
  exit_code = EX_UNAVAILABLE;
}

