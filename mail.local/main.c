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

int debug_level;
int multiple_delivery;
int ex_quota_tempfail;
int exit_code = EX_OK;
uid_t uid;
char *quotadbname = NULL;
int lock_timeout = 300;

#define MAXFD 64
#define EX_QUOTA() (ex_quota_tempfail ? EX_TEMPFAIL : EX_UNAVAILABLE)

void close_fds ();
int switch_user_id (uid_t uid);
FILE *make_tmp (const char *from, char **tempfile);
void deliver (FILE *fp, char *name);
void guess_retval (int ec);
void mailer_err (char *fmt, ...);
void notify_biff (mailbox_t mbox, char *name, size_t size);

const char *argp_program_version = "mail.local (" PACKAGE ") " VERSION;
const char *argp_program_bug_address = "<bug-mailutils@gnu.org>";
static char doc[] = "GNU mail.local -- the local MDA";
static char args_doc[] = "recipient [recipient ...]";

#define ARG_MULTIPLE_DELIVERY 1
#define ARG_QUOTA_TEMPFAIL 2

static struct argp_option options[] = 
{
  {NULL, 0, NULL, 0,
   "mail.local specific switches", 0},
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
#ifdef WITH_GUILE
  { "source", 's', "PATTERN", 0,
    "Set name pattern for user-defined mail filters", 0 },
#endif
  { "debug", 'x',
#ifdef WITH_GUILE
    "{NUMBER|guile}",
#else
    "NUMBER",
#endif
    0,
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
  "mailutils",
  "logging",
  NULL
};

char *from = NULL;
char *progfile_pattern = NULL;

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
	  mu_error ("multiple --from options");
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
	
      case 't':
	lock_timeout = strtoul (optarg, NULL, 0);
	break;
	
      case 'x':
	if (optarg)
	  {
#ifdef WITH_GUILE
	    if (strcmp (optarg, "guile") == 0)
	      debug_guile = 1;
	    else
#endif
	      debug_level = strtoul (optarg, NULL, 0);
	  }
	else
	  {
	    debug_level = 9;
#ifdef WITH_GUILE	    
	    debug_guile = 1;
#endif
	  }
	break;

    default:
      return ARGP_ERR_UNKNOWN;

    case ARGP_KEY_ERROR:
      exit (EX_USAGE);
    }
  return 0;
}

int
main (int argc, char *argv[])
{
  FILE *fp;
  char *tempfile = NULL;
  int arg_index;
  
  /* Preparative work: close inherited fds, force a reasonable umask
     and prepare a logging. */
  close_fds ();
  umask (0077);

  mu_argp_error_code = EX_CONFIG;
  
  mu_argp_parse (&argp, &argc, &argv, 0, argp_capa, &arg_index, NULL);
  
  openlog ("mail.local", LOG_PID, log_facility);
  mu_error_set_print (mu_syslog_error_printer);
  
  uid = getuid ();

  argc -= arg_index;
  argv += arg_index;

  if (!argc)
    {
      mu_error ("Missing arguments. Try --help for more info");
      return EX_USAGE;
    }

#ifdef HAVE_MYSQL
  mu_register_getpwnam (getMpwnam);
  mu_register_getpwuid (getMpwuid);
#endif
#ifdef USE_VIRTUAL_DOMAINS
  mu_register_getpwnam (getpwnam_virtual);
  mu_register_getpwnam (getpwnam_ip_virtual);
  mu_register_getpwnam (getpwnam_host_virtual);
#endif

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

  fp = make_tmp (from, &tempfile);
  
  if (multiple_delivery)
    multiple_delivery = argc > 1;

#ifdef WITH_GUILE
  if (progfile_pattern)
    {
      struct mda_data mda_data;
      
      memset (&mda_data, 0, sizeof mda_data);
      mda_data.fp = fp;
      mda_data.argv = argv;
      mda_data.progfile_pattern = progfile_pattern;
      mda_data.tempfile = tempfile;
      return prog_mda (&mda_data);
    }
#endif
  
  unlink (tempfile);
  for (; *argv; argv++)
    mda (fp, *argv, NULL);
  return exit_code;
}

char *
make_progfile_name (char *pattern, char *username)
{
  char *homedir = NULL;
  char *p, *q, *startp;
  char *progfile;
  int len = 0;
  
  for (p = pattern; *p; p++)
    {
      if (*p == '%')
	switch (*++p)
	  {
	  case 'u':
	    len += strlen (username);
	    break;
	  case 'h':
	    if (!homedir)
	      {
		struct passwd *pwd = mu_getpwnam (username);
		if (!pwd)
		  return NULL;
		homedir = pwd->pw_dir;
	      }
	    len += strlen (homedir);
	    break;
	  case '%':
	    len++;
	    break;
	  default:
	    len += 2;
	  }
      else
	len++;
    }
  
  progfile = malloc (len + 1);
  if (!progfile)
    return NULL;

  startp = pattern;
  q = progfile;
  while (*startp && (p = strchr (startp, '%')) != NULL)
    {
      memcpy (q, startp, p - startp);
      q += p - startp;
      switch (*++p)
	{
	case 'u':
	  strcpy (q, username);
	  q += strlen (username);
	  break;
	case 'h':
	  strcpy (q, homedir);
	  q += strlen (homedir);
	  break;
	case '%':
	  *q++ = '%';
	  break;
	default:
	  *q++ = '%';
	  *q++ = *p;
	}
      startp = p + 1;
    }
  if (*startp)
    {
      strcpy (q, startp);
      q += strlen (startp);
    }
  *q = 0;
  return progfile;
}

int
mda (FILE *fp, char *username, mailbox_t mbox)
{
  if (mbox)
    {
      message_t mesg = NULL;
      attribute_t attr = NULL;
      
      mailbox_get_message (mbox, 1, &mesg);
      message_get_attribute (mesg, &attr);
      if (attribute_is_deleted (attr))
	return EX_OK;
    }

  deliver (fp, username);

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
switch_user_id (uid_t uid)
{
  int rc;
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

FILE *
make_tmp (const char *from, char **tempfile)
{
  time_t t;
  int fd = mu_tempfile (NULL, tempfile);
  FILE *fp;
  char *buf = NULL;
  size_t n = 0;
  int line;
  
  if (fd == -1 || (fp = fdopen (fd, "w+")) == NULL)
    {
      mailer_err ("unable to open temporary file");
      exit (exit_code);
    }

  line = 0;
  while (getline (&buf, &n, stdin) > 0) {
    line++;
    if (line == 1)
      {
	if (memcmp (buf, "From ", 5))
	  {
	    if (!from)
	      {
		struct passwd *pw = mu_getpwuid (uid);
		if (pw)
		  from = pw->pw_name;
	      }
	    if (from)
	      {
		time (&t);
		fprintf (fp, "From %s %s", from, ctime (&t));
	      }
	    else
	      {
		mailer_err ("Can't determine sender address");
		exit (EX_UNAVAILABLE);
	      }
	  }
      }
    else if (!memcmp (buf, "From ", 5))
      fputc ('>', fp);
    if (fputs (buf, fp) == EOF)
      {
	mailer_err ("temporary file write error");
	fclose (fp);
	return NULL;
      }
  }

  if (buf && strchr (buf, '\n') == NULL)
    putc ('\n', fp);

  putc ('\n', fp);
  free (buf);
  
  return fp;
}

void
deliver (FILE *fp, char *name)
{
  mailbox_t mbox;
  char *path;
  url_t url = NULL;
  size_t n = 0;
  locker_t lock;
  struct passwd *pw;
  int status;
  stream_t stream;
  size_t size;
  int failed = 0;
#if defined(USE_DBM)
  struct stat sb;
#endif  
  
  pw = mu_getpwnam (name);
  if (!pw)
    {
      mailer_err ("%s: no such user", name);
      exit_code = EX_UNAVAILABLE;
      return;
    }
  
  path = malloc (strlen (mu_path_maildir) + strlen (name) + 1);
  if (!path)
    {
      mailer_err ("Out of memory");
      return;
    }
  sprintf (path, "%s%s", mu_path_maildir, name);

  if ((status = mailbox_create (&mbox, path)) != 0)
    {
      mailer_err ("can't open mailbox %s: %s", path, mu_errstring (status));
      free (path);
      return;
    }

  free (path);
      
  mailbox_get_url (mbox, &url);
  path = (char*) url_to_string (url);

  /* Actually open the mailbox. Switch to the user's euid to make
     sure the maildrop file will have right privileges, in case it
     will be created */
  if (switch_user_id (pw->pw_uid))
    return;
  status = mailbox_open (mbox, MU_STREAM_RDWR|MU_STREAM_CREAT);
  if (switch_user_id (0))
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

  if ((status = mailbox_get_stream (mbox, &stream)))
    {
      mailer_err ("can't get stream for mailbox %s: %s",
		  path, mu_errstring (status));
      mailbox_destroy (&mbox);
      return;
    }

  if ((status = stream_size (stream, (off_t *) &size)))
    {
      mailer_err ("can't get stream size (mailbox %s): %s",
		  path, mu_errstring (status));
      mailbox_destroy (&mbox);
      return;
    }

#if defined(USE_DBM)
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
      if (fstat (fileno (fp), &sb))
	{
	  mailer_err ("cannot stat tempfile: %s", strerror (errno));
	  exit_code = EX_UNAVAILABLE;
	  failed++;
	}
      else if (sb.st_size > n)
	{
	  mailer_err ("%s: message would exceed maximum mailbox size for this recipient",
		      name);
	  exit_code = EX_QUOTA();
	  failed++;
	}
      break;
    }
#endif
  
  if (!failed && switch_user_id (pw->pw_uid) == 0)
    {
      off_t off = size;
      size_t nwr;
      char *buf = NULL;

      n = 0;
      status = 0;
      fseek (fp, 0, SEEK_SET);
      while (getline(&buf, &n, fp) > 0) {
	status = stream_write (stream, buf, strlen (buf), off, &nwr);
	if (status)
	  {
	    mailer_err ("error writing to mailbox: %s", mu_errstring (status));
	    break;
	  }
	off += nwr;
      }
      free (buf);
      switch_user_id (0);
    }

  if (!failed)
    notify_biff (mbox, name, size);

  locker_unlock (lock);

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





