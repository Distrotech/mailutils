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

#include <mail.local.h>

int multiple_delivery;
int ex_quota_tempfail;
int exit_code = EX_OK;
uid_t uid;
char *quotadbname = NULL;
int lock_timeout = 300;

/* Debuggig options */
int debug_level;
int debug_flags;
int sieve_debug_flags;
int sieve_enable_log;
mu_debug_t mudebug;

#define MAXFD 64
#define EX_QUOTA() (ex_quota_tempfail ? EX_TEMPFAIL : EX_UNAVAILABLE)

void close_fds __P((void));
int make_tmp __P((const char *from, mailbox_t *mbx));
void deliver __P((mailbox_t msg, char *name));
void guess_retval __P((int ec));
void mailer_err __P((char *fmt, ...));
void notify_biff __P((mailbox_t mbox, char *name, size_t size));

const char *argp_program_version = "mail.local (" PACKAGE_STRING ")";
static char doc[] =
"GNU mail.local -- the local MDA"
"\v"
"Debug flags are:\n"
"  g - guimb stack traces\n"
"  T - mailutil traces (MU_DEBUG_TRACE)\n"
"  P - network protocols (MU_DEBUG_PROT)\n"
"  t - sieve trace (MU_SIEVE_DEBUG_TRACE)\n"
"  l - sieve action logs\n"
"  0-9 - Set mail.local debugging level\n";

static char args_doc[] = "recipient [recipient ...]";

#define ARG_MULTIPLE_DELIVERY 1
#define ARG_QUOTA_TEMPFAIL 2

static struct argp_option options[] = 
{
  { "ex-multiple-delivery-success", ARG_MULTIPLE_DELIVERY, NULL, 0,
    "Don't return errors when delivering to multiple recipients", 0 },
  { "ex-quota-tempfail", ARG_QUOTA_TEMPFAIL, NULL, 0,
    "Return temporary failure if disk or mailbox quota is exceeded", 0 },
  { "from", 'f', "EMAIL", 0,
    "Specify the sender's name" },
  { NULL, 'r', NULL, OPTION_ALIAS, NULL },
#ifdef USE_DBM
  { "quota-db", 'q', "FILE", 0,
    "Specify path to quota database", 0 },
#endif
  { "sieve", 'S', "PATTERN", 0,
    "Set name pattern for user-defined sieve mail filters", 0 },
#ifdef WITH_GUILE
  { "source", 's', "PATTERN", 0,
    "Set name pattern for user-defined mail filters", 0 },
#endif
  { "debug", 'x', "FLAGS", 0,
    "Enable debugging", 0 },
  { "timeout", 't', "NUMBER", 0,
    "Set timeout for acquiring the lockfile" },
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
  "sieve",
  NULL
};

char *from = NULL;
char *progfile_pattern = NULL;
char *sieve_pattern = NULL;

#define D_DEFAULT "9s"

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

    case 'r':
    case 'f':
      if (from != NULL)
	{
	  argp_error (state, "multiple --from options");
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
      progfile_pattern = optarg;
      break;
#endif

    case 'S':
      sieve_pattern = optarg;
      break;
      
    case 't':
      lock_timeout = strtoul (optarg, NULL, 0);
      break;
	
    case 'x':
      do
	{
	  if (!optarg)
	    optarg = D_DEFAULT;
	  switch (*optarg)
	    {
	    case 'g':
#ifdef WITH_GUILE
	      debug_guile = 1;
#endif
	      break;

	    case 't':
	      sieve_debug_flags |= MU_SIEVE_DEBUG_TRACE;
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
	      if (isdigit (*optarg))
		debug_level = *optarg - '0';
	      else
		argp_error (state, "%c is not a valid debug flag", *arg);
	      break;
	    }
	}
      while (*++optarg);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_ERROR:
      exit (EX_USAGE);
    }
  return 0;
}


static int
_mu_debug_printer (mu_debug_t unused, size_t level, const char *fmt,
		   va_list ap)
{
  vsyslog ((level == MU_DEBUG_ERROR) ? LOG_ERR : LOG_DEBUG, fmt, ap);
  return 0;
}

static int
_sieve_debug_printer (void *unused, const char *fmt, va_list ap)
{
  vsyslog (LOG_DEBUG, fmt, ap);
  return 0;
}

static void
_sieve_action_log (void *user_name,
		   const char *script, size_t msgno, message_t msg,
		   const char *action, const char *fmt, va_list ap)
{
  size_t uid = 0;
  char *text = NULL;
  
  message_get_uid (msg, &uid);

  asprintf (&text, "%s on msg uid %d", action, uid);
  if (fmt && strlen (fmt))
    {
      char *diag = NULL;
      vasprintf (&diag, fmt, ap);
      syslog (LOG_NOTICE, "(user %s) %s: %s", (char*) user_name, text, diag);
      free (diag);
    }
  else
    syslog (LOG_NOTICE, "(user %s) %s", (char*) user_name, text);
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
    syslog (LOG_ERR, "(user %s) %s", (char*)user_name, text);
  free (text);
  return 0;
}

int
main (int argc, char *argv[])
{
  mailbox_t mbox = NULL;
  int arg_index;
  
  /* Preparative work: close inherited fds, force a reasonable umask
     and prepare a logging. */
  close_fds ();
  umask (0077);

  mu_argp_error_code = EX_CONFIG;
  MU_AUTH_REGISTER_ALL_MODULES();
  sieve_argp_init ();
  mu_argp_parse (&argp, &argc, &argv, 0, argp_capa, &arg_index, NULL);
  
  openlog ("mail.local", LOG_PID, log_facility);
  mu_error_set_print (mu_syslog_error_printer);
  if (debug_flags)
    {
      int rc;
      
      if ((rc = mu_debug_create (&mudebug, NULL)))
	{
	  mu_error ("mu_debug_create failed: %s\n", mu_errstring (rc));
	  exit (EX_TEMPFAIL);
	}
      if ((rc = mu_debug_set_level (mudebug, debug_flags)))
	{
	  mu_error ("mu_debug_set_level failed: %s\n",
		    mu_errstring (rc));
	  exit (EX_TEMPFAIL);
	}
      if ((rc = mu_debug_set_print (mudebug, _mu_debug_printer, NULL)))
	{
	  mu_error ("mu_debug_set_print failed: %s\n",
		    mu_errstring (rc));
	  exit (EX_TEMPFAIL);
	}
    }
  
  uid = getuid ();

  argc -= arg_index;
  argv += arg_index;

  if (!argc)
    {
      mu_error ("Missing arguments. Try --help for more info");
      return EX_USAGE;
    }

  /* Register local mbox formats. */
  {
    list_t bookie;
    registrar_get_list (&bookie);
    list_append (bookie, mbox_record); 
    list_append (bookie, path_record);
    /* Possible supported mailers.  */
    list_append (bookie, sendmail_record);
    list_append (bookie, smtp_record);
  }

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
sieve_test (struct mu_auth_data *auth, mailbox_t mbx)
{
  int rc = 1;
  char *progfile;
    
  if (!sieve_pattern)
    return 1;

  progfile = mu_expand_path_pattern (sieve_pattern, auth->name);
  if (access (progfile, R_OK))
    {
      if (debug_level > 2)
	syslog (LOG_DEBUG, "access to %s failed: %m", progfile);
    }
  else
    {
      sieve_machine_t mach;
      rc = sieve_machine_init (&mach, auth->name);
      if (rc)
	{
	  mu_error ("can't initialize sieve machine: %s",
		    mu_errstring (rc));
	}
      else
	{
	  sieve_set_debug (mach, _sieve_debug_printer);
	  sieve_set_debug_level (mach, mudebug, sieve_debug_flags);
	  sieve_set_parse_error (mach, _sieve_parse_error);
	  if (sieve_enable_log)
	    sieve_set_logger (mach, _sieve_action_log);
	  
	  rc = sieve_compile (mach, progfile);
	  if (rc == 0)
	    {
	      attribute_t attr;
	      message_t msg = NULL;
		
	      mailbox_get_message (mbx, 1, &msg);
	      message_get_attribute (msg, &attr);
	      attribute_unset_deleted (attr);
	      if (switch_user_id (auth, 1) == 0)
		{
		  chdir (auth->dir);
		
		  rc = sieve_message (mach, msg);
		  if (rc == 0)
		    rc = attribute_is_deleted (attr) == 0;

		  switch_user_id (auth, 0);
		  chdir ("/");
		}
	      sieve_machine_destroy (&mach);
	    }
	}
    }
  free (progfile);
  return rc;
}

int
mda (mailbox_t mbx, char *username)
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
    mailer_err ("setreuid(0, %d): %s (r=%d, e=%d)",
		uid, strerror (errno), getuid (), geteuid ());
  return rc;
}

static int
tmp_write (stream_t stream, off_t *poffset, char *buf, size_t len)
{
  size_t n = 0;
  int status = stream_write (stream, buf, len, *poffset, &n);
  if (status == 0 && n != len)
    status = EIO;
  *poffset += n;
  return status;
}

int
make_tmp (const char *from, mailbox_t *mbox)
{
  stream_t stream;
  char *buf = NULL;
  size_t n = 0;
  off_t offset = 0;
  size_t line;
  int status;
  static char *newline = "\n";
  char *tempfile;
  
  tempfile = mu_tempname (NULL);
  if ((status = file_stream_create (&stream, tempfile, MU_STREAM_RDWR)))
    {
      mailer_err ("unable to open temporary file: %s", mu_errstring (status));
      exit (exit_code);
    }

  if ((status = stream_open (stream)))
    {
      mailer_err ("unable to open temporary file: %s", mu_errstring (status));
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
		  char *ptr;
		  
		  time (&t);
		  asprintf (&ptr, "From %s %s", from, ctime (&t));
		  status = tmp_write (stream, &offset, ptr, strlen (ptr));
		  free (ptr);
		}
	      else
		{
		  mailer_err ("Can't determine sender address");
		  exit (EX_UNAVAILABLE);
		}
	      if (auth)
		mu_auth_data_free (auth);
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
	  mailer_err ("temporary file write error: %s", mu_errstring (status));
	  stream_destroy (&stream, stream_get_owner (stream));
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
      mailer_err ("temporary file write error: %s", mu_errstring (status));
      stream_destroy (&stream, stream_get_owner (stream));
      return status;
    }

  stream_flush (stream);
  if ((status = mailbox_create (mbox, "/dev/null")) 
      || (status = mailbox_open (*mbox, MU_STREAM_READ))
      || (status = mailbox_set_stream (*mbox, stream)))
    {
      mailer_err ("temporary file open error: %s", mu_errstring (status));
      stream_destroy (&stream, stream_get_owner (stream));
      return status;
    }

  status = mailbox_messages_count (*mbox, &n);
  if (status)
    {
      errno = status;
      mailer_err ("temporary message creation error: %s",
		  mu_errstring (status));
      stream_destroy (&stream, stream_get_owner (stream));
      return status;
    }

  return status;
}

void
deliver (mailbox_t imbx, char *name)
{
  mailbox_t mbox;
  char *path;
  url_t url = NULL;
  locker_t lock;
  struct mu_auth_data *auth;
  int status;
  stream_t istream, ostream;
  size_t size;
  int failed = 0;
  
  auth = mu_get_auth_by_name (name);
  if (!auth)
    {
      mailer_err ("%s: no such user", name);
      exit_code = EX_UNAVAILABLE;
      return;
    }

  if (!sieve_test (auth, imbx))
    {
      exit_code = EX_OK;
      mu_auth_data_free (auth);
      return;
    }

  if ((status = mailbox_get_stream (imbx, &istream)) != 0)
    {
      mailer_err ("can't get input message stream: %s", mu_errstring (status));
      mu_auth_data_free (auth);
      return;
    }
  
  if ((status = mailbox_create (&mbox, auth->mailbox)) != 0)
    {
      mailer_err ("can't open mailbox %s: %s",
		  auth->mailbox, mu_errstring (status));
      mu_auth_data_free (auth);
      return;
    }

  mailbox_get_url (mbox, &url);
  path = (char*) url_to_string (url);

  /* Actually open the mailbox. Switch to the user's euid to make
     sure the maildrop file will have right privileges, in case it
     will be created */
  if (switch_user_id (auth, 1))
    return;
  status = mailbox_open (mbox, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (switch_user_id (auth, 0))
    return;
  if (status != 0)
    {
      mailer_err ("can't open mailbox %s: %s", path, mu_errstring (status));
      mailbox_destroy (&mbox);
      return;
    }
  
  mailbox_get_locker (mbox, &lock);
  locker_set_flags (lock, MU_LOCKER_PID|MU_LOCKER_RETRY);
  locker_set_retries (lock, lock_timeout);

  status = locker_lock (lock);

  if (status)
    {
      mailer_err ("cannot lock mailbox '%s': %s", path, mu_errstring (status));
      mailbox_destroy (&mbox);
      exit_code = EX_TEMPFAIL;
      return;
    }

  if ((status = mailbox_get_stream (mbox, &ostream)))
    {
      mailer_err ("can't get stream for mailbox %s: %s",
		  path, mu_errstring (status));
      mailbox_destroy (&mbox);
      return;
    }

  if ((status = stream_size (ostream, (off_t *) &size)))
    {
      mailer_err ("can't get stream size (mailbox %s): %s",
		  path, mu_errstring (status));
      mailbox_destroy (&mbox);
      return;
    }

#if defined(USE_DBM)
  {
    size_t n, isize;

    switch (check_quota (name, size, &n))
      {
      case MQUOTA_EXCEEDED:
	mailer_err ("%s: mailbox quota exceeded for this recipient", name);
	exit_code = EX_QUOTA();
	failed++;
	break;
	
      case MQUOTA_UNLIMITED:
	break;
	
      default:
	if ((status = stream_size (istream, (off_t *) &isize)))
	  {
	    mailer_err ("can't get stream size (input message): %s",
			path, mu_errstring (status));
	    exit_code = EX_UNAVAILABLE;
	    failed++;
	  }
	else if (isize > n)
	  {
	    mailer_err ("%s: message would exceed maximum mailbox size for this recipient",
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
      off_t ioff = 0;
      off_t off = size;
      size_t nwr, nrd;
      char *buf = NULL;
      size_t bufsize = 1024;

      stream_size (istream, (off_t *) &bufsize);
      for (; (buf = malloc (bufsize)) == NULL && bufsize > 1; bufsize /= 2)
	;
      
      if (!buf)
	{
	  status = errno = ENOMEM;
	  failed++;
	}
      else
	{
	  status = 0;

	  while ((status = stream_read (istream, buf, bufsize, ioff, &nrd))
		 == 0
		 && nrd > 0)
	    {
	      status = stream_write (ostream, buf, nrd, off, &nwr);
	      if (status)
		break;
	      ioff += nrd;
	      off += nwr;
	    }
	  
	  free (buf);
	}
      
      switch_user_id (auth, 0);

      if (status)
	{
	  mailer_err ("error writing to mailbox: %s",
		      mu_errstring (status));
	}
    }

  if (!failed)
    notify_biff (mbox, name, size);

  locker_unlock (lock);

  mu_auth_data_free (auth);
  mailbox_close (mbox);
  mailbox_destroy (&mbox);
}

void
notify_biff (mailbox_t mbox, char *name, size_t size)
{
  static int fd = -1;
  url_t url = NULL;
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
      
      fd = socket (AF_INET, SOCK_DGRAM, 0);
      if (fd < 0)
	fd = -2; /* Mark failed initialization */
    }

  if (fd < 0)
    return;
  
  mailbox_get_url (mbox, &url);
  asprintf (&buf, "%s@%ld:%s", name, size, url_to_string (url));
  if (buf)
    {
      sendto (fd, buf, strlen (buf), 0, (struct sockaddr *)&inaddr,
	      sizeof inaddr);
      free (buf);
    }
}

void
mailer_err (char *fmt, ...)
{
  va_list ap;

  guess_retval (errno);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  mu_verror (fmt, ap);
  va_end (ap);
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





