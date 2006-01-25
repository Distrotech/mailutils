/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 2005
   Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <getline.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/argp.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/mutil.h>
#include <mailutils/locker.h>
#include <mailutils/mailer.h>
#include <mailutils/mailbox.h>
#include <mailutils/nls.h>
#include <mailutils/argcv.h>

#define ARG_LOG_FACILITY          1
#define ARG_LOCK_FLAGS            2
#define ARG_LOCK_RETRY_COUNT      3
#define ARG_LOCK_RETRY_TIMEOUT    4
#define ARG_LOCK_EXPIRE_TIMEOUT   5
#define ARG_LOCK_EXTERNAL_PROGRAM 6
#define ARG_SHOW_OPTIONS          7
#define ARG_LICENSE               8
#define ARG_MAILBOX_TYPE          9

static struct argp_option mu_common_argp_options[] = 
{
  { NULL, 0, NULL, 0, N_("Common options"), 0},
  { "show-config-options", ARG_SHOW_OPTIONS, NULL, OPTION_HIDDEN,
    N_("Show compilation options"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Option to print the license. */
static struct argp_option mu_license_argp_option[] = {
  { "license", ARG_LICENSE, NULL, 0, N_("Print license and exit"), -2 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that access mailboxes. */
static struct argp_option mu_mailbox_argp_option[] = {
  {"mail-spool", 'm', N_("URL"), 0,
   N_("Use specified URL as a mailspool directory"), 0},
  {"mailbox-type", ARG_MAILBOX_TYPE, N_("PROTO"), 0,
   N_("Default mailbox type to use"), 0 },
  {"lock-flags", ARG_LOCK_FLAGS, N_("FLAGS"), 0,
   N_("Default locker flags (E=external, R=retry, T=time, P=pid)"), 0},
  {"lock-retry-timeout", ARG_LOCK_RETRY_TIMEOUT, N_("SECONDS"), 0,
   N_("Set timeout for acquiring the lockfile") },
  {"lock-retry-count", ARG_LOCK_RETRY_COUNT, N_("NUMBER"), 0,
   N_("Set the maximum number of times to retry acquiring the lockfile") },
  {"lock-expire-timeout", ARG_LOCK_EXPIRE_TIMEOUT, N_("SECONDS"), 0,
   N_("Number of seconds after which the lock expires"), },
  {"external-locker", ARG_LOCK_EXTERNAL_PROGRAM, N_("PATH"), 0,
   N_("Set full path to the external locker program") },
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that do address mapping. */
static struct argp_option mu_address_argp_option[] = {
  {"email-addr", 'E', N_("EMAIL"), 0,
   N_("Set current user's email address (default is loginname@defaultdomain)"), 0},
  {"email-domain", 'D', N_("DOMAIN"), 0,
   N_("Set domain for unqualified user names (default is this host)"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that send mail. */
static struct argp_option mu_mailer_argp_option[] = {
  {"mailer", 'M', N_("MAILER"), 0,
   N_("Use specified URL as the default mailer"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that log to syslog. */
static struct argp_option mu_logging_argp_option[] = {
  {"log-facility", ARG_LOG_FACILITY, N_("FACILITY"), 0,
   N_("Output logs to syslog FACILITY"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};


/* Options used by programs that become daemons. */
static struct argp_option mu_daemon_argp_option[] = {
  {"daemon", 'd', N_("NUMBER"), OPTION_ARG_OPTIONAL,
   N_("Runs in daemon mode with a maximum of NUMBER children")},
  {"inetd",  'i', 0, 0,
   N_("Run in inetd mode"), 0},
  {"port", 'p', N_("PORT"), 0,
   N_("Listen on specified port number"), 0},
  {"timeout", 't', N_("NUMBER"), 0,
   N_("Set idle timeout value to NUMBER seconds"), 0},
  {"transcript", 'x', NULL, 0,
   N_("Output session transcript via syslog"), 0},
  {"pidfile", 'P', N_("FILE"), 0,
   N_("Set PID file"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t mu_common_argp_parser (int key, char *arg,
				      struct argp_state *state);
static error_t mu_daemon_argp_parser (int key, char *arg,
				      struct argp_state *state);

struct argp mu_common_argp = {
  mu_common_argp_options,
  mu_common_argp_parser,
};

struct argp_child mu_common_argp_child = {
  &mu_common_argp,
  0,
  NULL,
  0,
};

struct argp mu_license_argp = {
  mu_license_argp_option,
  mu_common_argp_parser,
};

struct argp_child mu_license_argp_child = {
  &mu_license_argp,
  0,
  NULL,
  0
};

struct argp mu_mailbox_argp = {
  mu_mailbox_argp_option,
  mu_common_argp_parser,
};

struct argp_child mu_mailbox_argp_child = {
  &mu_mailbox_argp,
  0,
  NULL,
  0
};

struct argp mu_address_argp = {
  mu_address_argp_option,
  mu_common_argp_parser,
};

struct argp_child mu_address_argp_child = {
  &mu_address_argp,
  0,
  NULL,
  0
};

struct argp mu_mailer_argp = {
  mu_mailer_argp_option,
  mu_common_argp_parser,
};

struct argp_child mu_mailer_argp_child = {
  &mu_mailer_argp,
  0,
  NULL,
  0
};

struct argp mu_logging_argp = {
  mu_logging_argp_option,
  mu_common_argp_parser,
};

struct argp_child mu_logging_argp_child = {
  &mu_logging_argp,
  0,
  NULL,
  0
};

struct argp mu_daemon_argp = {
  mu_daemon_argp_option,
  mu_daemon_argp_parser,
};

struct argp_child mu_daemon_argp_child = {
  &mu_daemon_argp,
  0,
  N_("Daemon configuration options"),
  0
};

int log_facility = LOG_FACILITY;
int mu_argp_error_code = 1;

static int
parse_log_facility (const char *str)
{
  int i;
  static struct {
    char *name;
    int facility;
  } syslog_kw[] = {
    { "USER",    LOG_USER },   
    { "DAEMON",  LOG_DAEMON },
    { "AUTH",    LOG_AUTH },  
    { "LOCAL0",  LOG_LOCAL0 },
    { "LOCAL1",  LOG_LOCAL1 },
    { "LOCAL2",  LOG_LOCAL2 },
    { "LOCAL3",  LOG_LOCAL3 },
    { "LOCAL4",  LOG_LOCAL4 },
    { "LOCAL5",  LOG_LOCAL5 },
    { "LOCAL6",  LOG_LOCAL6 },
    { "LOCAL7",  LOG_LOCAL7 },
    { "MAIL",    LOG_MAIL }
  };

  if (strncmp (str, "LOG_", 4) == 0)
    str += 4;

  for (i = 0; i < sizeof (syslog_kw) / sizeof (syslog_kw[0]); i++)
    if (strcasecmp (syslog_kw[i].name, str) == 0)
      return syslog_kw[i].facility;
  fprintf (stderr, _("Unknown facility `%s'\n"), str);
  return LOG_FACILITY;
}

char *mu_license_text =
 N_("   GNU Mailutils is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 2 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils; if not, write to the Free Software Foundation,\n"
    "   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
    "\n"
    "\n"
);

static char *mu_conf_option[] = {
  "VERSION=" VERSION,
#ifdef USE_LIBPAM
  "USE_LIBPAM",
#endif
#ifdef HAVE_LIBLTDL
  "HAVE_LIBLTDL",
#endif
#ifdef WITH_BDB2
  "WITH_BDB2",
#endif
#ifdef WITH_NDBM
  "WITH_NDBM",
#endif
#ifdef WITH_OLD_DBM
  "WITH_OLD_DBM",
#endif
#ifdef WITH_GDBM
  "WITH_GDBM",
#endif
#ifdef WITH_GNUTLS
  "WITH_GNUTLS",
#endif
#ifdef WITH_GSASL
  "WITH_GSASL",
#endif
#ifdef WITH_GSSAPI
  "WITH_GSSAPI",
#endif
#ifdef WITH_GUILE
  "WITH_GUILE",
#endif
#ifdef WITH_PTHREAD
  "WITH_PTHREAD",
#endif
#ifdef WITH_READLINE
  "WITH_READLINE",
#endif
#ifdef HAVE_MYSQL
  "HAVE_MYSQL",
#endif
#ifdef HAVE_PGSQL
  "HAVE_PGSQL",
#endif
#ifdef ENABLE_VIRTUAL_DOMAINS
  "ENABLE_VIRTUAL_DOMAINS",
#endif
#ifdef ENABLE_IMAP
  "ENABLE_IMAP",
#endif
#ifdef ENABLE_POP
  "ENABLE_POP", 
#endif
#ifdef ENABLE_MH
  "ENABLE_MH",
#endif
#ifdef ENABLE_MAILDIR
  "ENABLE_MAILDIR",
#endif
#ifdef ENABLE_SMTP
  "ENABLE_SMTP",
#endif
#ifdef ENABLE_SENDMAIL
  "ENABLE_SENDMAIL",
#endif
#ifdef ENABLE_NNTP
  "ENABLE_NNTP",
#endif
#ifdef ENABLE_RADIUS
  "ENABLE_RADIUS",
#endif
#ifdef WITH_INCLUDED_LIBINTL
  "WITH_INCLUDED_LIBINTL",
#endif
  NULL
};

void
mu_print_options ()
{
  int i;
  
  for (i = 0; mu_conf_option[i]; i++)
    printf ("%s\n", mu_conf_option[i]);
}

const char *
mu_check_option (char *name)
{
  int i;
  
  for (i = 0; mu_conf_option[i]; i++)
    {
      int len;
      char *q, *p = strchr (mu_conf_option[i], '=');
      if (p)
	len = p - mu_conf_option[i];
      else
	len = strlen (mu_conf_option[i]);

      if (strncasecmp (mu_conf_option[i], name, len) == 0) 
	return mu_conf_option[i];
      else if ((q = strchr (mu_conf_option[i], '_')) != NULL
	       && strncasecmp (q + 1, name, len - (q - mu_conf_option[i]) - 1) == 0)
	return mu_conf_option[i];
    }
  return NULL;
}  

static error_t
mu_common_argp_parser (int key, char *arg, struct argp_state *state)
{
  int err = 0;

  switch (key)
    {
      /* common */
    case ARG_LICENSE:
      printf (_("License for %s:\n\n"), argp_program_version);
      printf ("%s", mu_license_text);
      exit (0);

    case ARG_SHOW_OPTIONS:
      mu_print_options ();
      exit (0);
      
      /* mailbox */
    case 'm':
      err = mu_set_mail_directory (arg);
      if (err)
	argp_error (state, _("Cannot set mail directory name: %s"),
		    mu_strerror (err));
      break;

    case ARG_MAILBOX_TYPE:
      if (mu_mailbox_set_default_proto (arg))
	argp_error (state, _("Invalid mailbox type: %s"), arg);
      break;
      
    case ARG_LOCK_FLAGS:
      {
	int flags = 0;
	for ( ; *arg; arg++)
	  {
	    switch (*arg)
	      {
	      case 'E':
		flags |= MU_LOCKER_EXTERNAL;
		break;
		
	      case 'R':
		flags |= MU_LOCKER_RETRY;
		break;
		
	      case 'T':
		flags |= MU_LOCKER_TIME;
		break;
		
	      case 'P':
		flags |= MU_LOCKER_PID;
		break;
		
	      default:
		argp_error (state, _("Invalid lock flag `%c'"), *arg);
	      }
	  }
	mu_locker_set_default_flags (flags, mu_locker_assign);
      }
      break;

    case ARG_LOCK_RETRY_COUNT:
      {
	size_t t = strtoul (arg, NULL, 0);
	mu_locker_set_default_retry_count (t);
	mu_locker_set_default_flags (MU_LOCKER_RETRY, mu_locker_set_bit);
      }
      break;
	  
    case ARG_LOCK_RETRY_TIMEOUT:
      {
	time_t t = strtoul (arg, NULL, 0);
	mu_locker_set_default_retry_timeout (t);
	mu_locker_set_default_flags (MU_LOCKER_RETRY, mu_locker_set_bit);
      }
      break;

    case ARG_LOCK_EXPIRE_TIMEOUT:
      {
	time_t t = strtoul (arg, NULL, 0);
	mu_locker_set_default_expire_timeout (t);
	mu_locker_set_default_flags (MU_LOCKER_EXTERNAL, mu_locker_set_bit);
      }
      break;

    case ARG_LOCK_EXTERNAL_PROGRAM:
      mu_locker_set_default_external_program (arg);
      mu_locker_set_default_flags (MU_LOCKER_TIME, mu_locker_set_bit);
      break;
      
      /* address */
    case 'E':
      if ((err = mu_set_user_email(arg)) != 0)
	  {
	    argp_error (state, _("Invalid email address `%s': %s"),
			arg, mu_strerror(err));
	  }
      break;

    case 'D':
      if ((err = mu_set_user_email_domain(arg)) != 0)
	  {
	    argp_error (state, _("Invalid email domain `%s': %s"),
		arg, mu_strerror(err));
	  }
      break;

      /* mailer */
    case 'M':
      if ((err = mu_mailer_set_url_default (arg)) != 0)
	  {
	    argp_error (state, _("Invalid mailer URL `%s': %s"),
			arg, mu_strerror(err));
	  }
      break;

      /* log */
    case ARG_LOG_FACILITY:
      log_facility = parse_log_facility (arg);
      break;

    case ARGP_KEY_FINI:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static error_t
mu_daemon_argp_parser (int key, char *arg, struct argp_state *state)
{
  struct daemon_param *p = state->input;
  if (!p)
    return ARGP_ERR_UNKNOWN;
  switch (key)
    {
    case 'd':
      p->mode = MODE_DAEMON;
      if (arg)
        {
          size_t n = strtoul (arg, NULL, 10);
          if (n > 0)
            p->maxchildren = n;
        }
      break;

    case 'i':
      p->mode = MODE_INTERACTIVE;
      break;
      
    case 'p':
      p->mode = MODE_DAEMON;
      p->port = strtoul (arg, NULL, 10);
      break;

    case 'P':
      p->pidfile = arg;
      break;

    case 't':
      p->timeout = strtoul (arg, NULL, 10);
      break;

    case 'x':
      p->transcript = 1;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

#ifndef MU_CONFIG_FILE
# define MU_CONFIG_FILE SYSCONFDIR "/mailutils.rc"
#endif

#ifndef MU_USER_CONFIG_FILE
# define MU_USER_CONFIG_FILE "~/.mailutils"
#endif

static int
member (const char *array[], const char *text, size_t len)
{
  int i;
  for (i = 0; array[i]; i++)
    if (strncmp (array[i], text, len) == 0)
      return 1;
  return 0;
}

/*
Appends applicable options found in file NAME to argv. If progname
is NULL, all the options found are assumed to apply. Otherwise they
apply only if the line starts with ":something", and something is
found in the CAPA array, or the line starts with PROGNAME.
*/
void
read_rc (const char *progname, const char *name, const char *capa[],
	 int *argc, char ***argv)
{
  FILE *fp;
  char *linebuf = NULL;
  char *buf = NULL;
  size_t n = 0;
  int x_argc = *argc;
  char **x_argv = *argv;
  char* rcfile = mu_tilde_expansion (name, "/", NULL);

  if (!rcfile)
    return;
  
  fp = fopen (rcfile, "r");
  if (!fp)
    {
      free(rcfile);
      return;
    }
  
  while (getline (&buf, &n, fp) > 0)
    {
      char *kwp, *p;
      int len;
      
      for (kwp = buf; *kwp && isspace (*kwp); kwp++)
	;

      if (*kwp == '#' || *kwp == 0)
	continue;

      len = strlen (kwp);
      if (kwp[len-1] == '\n')
	kwp[--len] = 0;

      if (kwp[len-1] == '\\' || linebuf)
	{
	  int cont;
	  
	  if (kwp[len-1] == '\\')
	    {
	      kwp[--len] = 0;
	      cont = 1;
	    }
	  else
	    cont = 0;
	  
	  if (!linebuf)
	    linebuf = calloc (len + 1, 1);
	  else
	    linebuf = realloc (linebuf, strlen (linebuf) + len + 1);
	  
	  if (!linebuf)
	    {
	      fprintf (stderr, _("%s: not enough memory\n"), progname);
	      exit (1);
	    }
	  
	  strcpy (linebuf + strlen (linebuf), kwp);
	  if (cont)
	    continue;
	  kwp = linebuf;
	}

      len = 0;
      if(progname)
	{
	  for (p = kwp; *p && !isspace (*p); p++)
	    len++;
	}
      else
	p = kwp; /* Use the whole line. */

      if (progname == NULL
	  || (kwp[0] == ':' && member (capa, kwp+1, len-1))
	  || strncmp (progname, kwp, len) == 0
	  )
	{
	  int i, n_argc = 0;
	  char **n_argv;
              
	  if (mu_argcv_get (p, "", NULL, &n_argc, &n_argv))
	    {
	      mu_argcv_free (n_argc, n_argv);
	      if (linebuf)
		free (linebuf);
	      linebuf = NULL;
	      continue;
	    }
	  x_argv = realloc (x_argv,
			    (x_argc + n_argc) * sizeof (x_argv[0]));
	  if (!x_argv)
	    {
	      fprintf (stderr, _("%s: not enough memory\n"), progname);
	      exit (1);
	    }
	  
	  for (i = 0; i < n_argc; i++)
	    x_argv[x_argc++] = mu_tilde_expansion (n_argv[i], "/", NULL);
	  
	  free (n_argv);
	}
      if (linebuf)
	free (linebuf);
      linebuf = NULL;
    }
  fclose (fp);
  free(rcfile);

  *argc = x_argc;
  *argv = x_argv;
}


void
mu_create_argcv (const char *capa[],
		 int argc, char **argv, int *p_argc, char ***p_argv)
{
  char *progname;
  int x_argc;
  char **x_argv;
  int i;
  int rcdir = 0;

  progname = strrchr (argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  x_argv = malloc (sizeof (x_argv[0]));
  if (!x_argv)
    {
      fprintf (stderr, _("%s: not enough memory\n"), progname);
      exit (1);
    }

  /* Add command name */
  x_argc = 0;
  x_argv[x_argc] = argv[x_argc];
  x_argc++;

  /* Add global config file. */
  read_rc (progname, MU_CONFIG_FILE, capa, &x_argc, &x_argv);

  /* Look for per-user config files in ~/.mailutils/ or in ~/, but
     not both. This allows mailutils' utilities to have their config
     files segregated, if necessary. */

  {
    struct stat s;
    char* rcdirname = mu_tilde_expansion (MU_USER_CONFIG_FILE, "/", NULL);

    if (!rcdirname
	|| (stat(rcdirname, &s) == 0 && S_ISDIR(s.st_mode)))
      rcdir = 1;

    free(rcdirname);
  }

  /* Add per-user config file. */
  if (!rcdir)
    {
      read_rc (progname, MU_USER_CONFIG_FILE, capa, &x_argc, &x_argv);
    }
  else
    {
      char *userrc = NULL;

      userrc = malloc (sizeof (MU_USER_CONFIG_FILE) /* provides an extra slot
						       for null byte as well */
		       + 1 /* slash */
		       + 9 /*mailutils*/); 

      if (!userrc)
	{
	  fprintf (stderr, _("%s: not enough memory\n"), progname);
	  exit (1);
	}
      
      sprintf(userrc, "%s/mailutils", MU_USER_CONFIG_FILE);
      read_rc (progname, userrc, capa, &x_argc, &x_argv);
      
      free(userrc);
    }

  /* Add per-user, per-program config file. */
  {
    char *progrc = NULL;
    int size;
    
    if (rcdir)
      size = sizeof (MU_USER_CONFIG_FILE)
	             + 1
		     + strlen (progname)
		     + 2 /* rc */;
    else
      size = 6 /*~/.mu.*/
	     + strlen (progname)
	     + 3 /* "rc" + null terminator */;

    progrc = malloc (size);

    if (!progrc)
      {
	fprintf (stderr, _("%s: not enough memory\n"), progname);
	exit (1);
      }

    if (rcdir)
      sprintf (progrc, "%s/%src", MU_USER_CONFIG_FILE, progname);
    else
      sprintf (progrc, "~/.mu.%src", progname);

    read_rc (NULL, progrc, capa, &x_argc, &x_argv);
    free (progrc);
  }

  /* Finally, add the command line options */
  x_argv = realloc (x_argv, (x_argc + argc) * sizeof (x_argv[0]));
  for (i = 1; i < argc; i++)
    x_argv[x_argc++] = argv[i];

  x_argv[x_argc] = NULL;

  *p_argc = x_argc;
  *p_argv = x_argv;
}

#define MU_MAX_CAPA 24

struct argp_capa {
  char *capability;
  struct argp_child *child;
} mu_argp_capa[MU_MAX_CAPA] = {
  {"common",  &mu_common_argp_child},
  {"license", &mu_license_argp_child},
  {"mailbox", &mu_mailbox_argp_child},
  {"address", &mu_address_argp_child},
  {"mailer",  &mu_mailer_argp_child},
  {"logging", &mu_logging_argp_child},
  {"daemon",  &mu_daemon_argp_child},
  {NULL,}
};

int
mu_register_capa (const char *name, struct argp_child *child)
{
  int i;
  
  for (i = 0; i < MU_MAX_CAPA; i++)
    if (mu_argp_capa[i].capability == NULL)
      {
	mu_argp_capa[i].capability = strdup (name);
	mu_argp_capa[i].child = child;
	return 0;
      }
  return 1;
}


static struct argp_child *
find_argp_child (const char *capa)
{
  int i;
  for (i = 0; mu_argp_capa[i].capability; i++)
    if (strcmp (mu_argp_capa[i].capability, capa) == 0)
      return mu_argp_capa[i].child;
  return NULL;
}

static struct argp *
mu_build_argp (const struct argp *template, const char *capa[])
{
  int n;
  int nchild;
  struct argp_child *ap;
  const struct argp_option *opt;
  struct argp *argp;
  int group = 0;

  /* Count the capabilities. */
  for (n = 0; capa && capa[n]; n++)
    ;
  if (template->children)
    for (; template->children[n].argp; n++)
      ;

  ap = calloc (n + 1, sizeof (*ap));
  if (!ap)
    {
      mu_error (_("Out of memory"));
      abort ();
    }

  /* Copy the template's children. */
  nchild = 0;
  if (template->children)
    for (n = 0; template->children[n].argp; n++, nchild++)
      ap[nchild] = template->children[n];

  /* Find next group number */
  for (opt = template->options;
       opt && ((opt->name && opt->key) || opt->doc); opt++)
    if (opt->group > group)
      group = opt->group;

  group++;
    
  /* Append any capabilities to the children or options, as appropriate. */
  for (n = 0; capa && capa[n]; n++)
    {
      struct argp_child *child = find_argp_child (capa[n]);
      if (!child)
	{
	  mu_error (_("INTERNAL ERROR: requested unknown argp capability %s (please report)"),
		    capa[n]);
	  abort ();
	}
      ap[nchild] = *child;
      ap[nchild].group = group++;
      nchild++;
    }
  ap[nchild].argp = NULL;

  /* Copy the template, and give it the expanded children. */
  argp = malloc (sizeof (*argp));
  if (!argp)
    {
      mu_error (_("Out of memory"));
      abort ();
    }

  memcpy (argp, template, sizeof (*argp));

  argp->children = ap;

  return argp;
}

void
mu_argp_init (const char *vers, const char *bugaddr)
{
  argp_program_version = vers ? vers : PACKAGE_STRING;
  argp_program_bug_address = bugaddr ? bugaddr : "<" PACKAGE_BUGREPORT ">";
}

error_t
mu_argp_parse(const struct argp *argp, 
	      int *pargc, char **pargv[],  
	      unsigned flags,
	      const char *capa[],
	      int *arg_index,     
	      void *input)
{
  error_t ret;
  const struct argp argpnull = { 0 };

  /* Make sure we have program version and bug address initialized */
  mu_argp_init (argp_program_version, argp_program_bug_address);

  if(!argp)
    argp = &argpnull;

  argp = mu_build_argp (argp, capa);
  mu_create_argcv (capa, *pargc, *pargv, pargc, pargv);
  ret = argp_parse (argp, *pargc, *pargv, flags, arg_index, input);
  free ((void*) argp->children);
  free ((void*) argp);
  return ret;
}

void
mu_auth_init ()
{
  extern struct argp_child mu_auth_argp_child;
  if (mu_register_capa ("auth", &mu_auth_argp_child))
    {
      mu_error (_("INTERNAL ERROR: cannot register argp capability auth (please report)"));
      abort ();
    }
}
