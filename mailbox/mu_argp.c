/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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
#include <xalloc.h>

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

#include <argcv.h>
#include <mu_asprintf.h>

#ifdef HAVE_MYSQL
# include "../MySql/MySql.h"
#endif

#define ARG_LOG_FACILITY 1
#define ARG_SQL_GETPWNAM 2
#define ARG_SQL_GETPWUID 3
#define ARG_SQL_GETPASS 4
#define ARG_SQL_HOST 5
#define ARG_SQL_USER 6
#define ARG_SQL_PASSWD 7
#define ARG_SQL_DB 8
#define ARG_SQL_PORT 9
#define ARG_PAM_SERVICE 10
#define ARG_LOCK_FLAGS 11

static struct argp_option mu_common_argp_options[] = 
{
  { NULL, 0, NULL, 0, "Common options:", 0},
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Option to print the license. */
static struct argp_option mu_license_argp_option[] = {
  { "license", 'L', NULL, 0, "Print license and exit", -2 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that access mailboxes. */
static struct argp_option mu_mailbox_argp_option[] = {
  {"mail-spool", 'm', "URL", 0,
   "Use specified URL as a mailspool directory", 0},
  {"lock-flags", ARG_LOCK_FLAGS, "FLAGS", 0,
   "Default locker flags (E=external, R=retry, T=time, P=pid)", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that do address mapping. */
static struct argp_option mu_address_argp_option[] = {
  {"email-addr", 'E', "EMAIL", 0,
   "Set current user's email address (default is loginname@defaultdomain)", 0},
  {"email-domain", 'D', "DOMAIN", 0,
   "Set domain for unqualified user names (default is this host)", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that send mail. */
static struct argp_option mu_mailer_argp_option[] = {
  {"mailer", 'M', "MAILER", 0,
   "Use specified URL as the default mailer", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that log to syslog. */
static struct argp_option mu_logging_argp_option[] = {
  {"log-facility", ARG_LOG_FACILITY, "FACILITY", 0,
   "Output logs to syslog FACILITY", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that use extended authentication mechanisms. */
static struct argp_option mu_auth_argp_option[] = {
#ifdef USE_LIBPAM
  { "pam-service", ARG_PAM_SERVICE, "STRING", 0,
    "Use STRING as PAM service name", 0},
#endif
#ifdef HAVE_MYSQL
  {"sql-getpwnam", ARG_SQL_GETPWNAM, "QUERY", 0,
   "SQL query to retrieve a passwd entry based on username", 0},
  {"sql-getpwuid", ARG_SQL_GETPWUID, "QUERY", 0,
   "SQL query to retrieve a passwd entry based on UID", 0},
  {"sql-getpass", ARG_SQL_GETPASS, "QUERY", 0,
   "SQL query to retrieve a password from the database", 0},
  {"sql-host", ARG_SQL_HOST, "HOSTNAME", 0,
   "Name or IP of MySQL server to connect to", 0},
  {"sql-user", ARG_SQL_USER, "NAME", 0,
   "SQL user name", 0},
  {"sql-passwd", ARG_SQL_PASSWD, "STRING", 0,
   "SQL connection password", 0},
  {"sql-db", ARG_SQL_DB, "STRING", 0,
   "Name of the database to connect to", 0},
  {"sql-port", ARG_SQL_PORT, "NUMBER", 0,
   "Port to use", 0},
#endif
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that become daemons. */
static struct argp_option mu_daemon_argp_option[] = {
  {"daemon", 'd', "NUMBER", OPTION_ARG_OPTIONAL,
   "Runs in daemon mode with a maximum of NUMBER children"},
  {"inetd",  'i', 0, 0,
   "Run in inetd mode", 0},
  {"port", 'p', "PORT", 0,
   "Listen on specified port number", 0},
  {"timeout", 't', "NUMBER", 0,
   "Set idle timeout value to NUMBER seconds", 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};  

static error_t mu_common_argp_parser __P((int key, char *arg,
					  struct argp_state *state));
static error_t mu_daemon_argp_parser __P((int key, char *arg,
					  struct argp_state *state));

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

struct argp mu_auth_argp = {
  mu_auth_argp_option,
  mu_common_argp_parser,
};

struct argp_child mu_auth_argp_child = {
  &mu_auth_argp,
  0,
  "Authentication options",
  0
};

struct argp mu_daemon_argp = {
  mu_daemon_argp_option,
  mu_daemon_argp_parser,
};

struct argp_child mu_daemon_argp_child = {
  &mu_daemon_argp,
  0,
  "Daemon configuration options",
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
  fprintf (stderr, "unknown facility `%s'\n", str);
  return LOG_FACILITY;
}

static char license_text[] =
    "   This program is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 2, or (at your option)\n"
    "   any later version.\n"
    "\n"
    "   This program is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License\n"
    "   along with this program; if not, write to the Free Software\n"
    "   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n";

#ifdef HAVE_MYSQL
char *sql_getpwnam_query;
char *sql_getpass_query;
char *sql_getpwuid_query;
char *sql_host = MHOST;
char *sql_user = MUSER;
char *sql_passwd = MPASS;
char *sql_db = MDB;
char *sql_socket = MSOCKET;
int  sql_port = MPORT;
#endif
#ifdef USE_LIBPAM
char *pam_service = NULL;
#endif

static error_t
mu_common_argp_parser (int key, char *arg, struct argp_state *state)
{
  int err = 0;
  char *p;

  switch (key)
    {
      /* common */
    case 'L':
      printf ("License for %s:\n\n", argp_program_version);
      printf ("%s", license_text);
      exit (0);

      /* mailbox */
    case 'm':
      mu_path_maildir = arg;
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
		argp_error (state, "invalid lock flag '%c'", *arg);
	      }
	  }
	locker_set_default_flags(flags);
      }
      break;

      /* address */
    case 'E':
      if ((err = mu_set_user_email(arg)) != 0)
	  {
	    argp_error (state, "invalid email-addr '%s': %s",
		arg, mu_errstring(err));
	  }
      break;

    case 'D':
      if ((err = mu_set_user_email_domain(arg)) != 0)
	  {
	    argp_error (state, "invalid email-domain '%s': %s",
		arg, mu_errstring(err));
	  }
      break;

      /* mailer */
    case 'M':
      if ((err = mailer_set_url_default(arg)) != 0)
	  {
	    argp_error (state, "invalid mailer url '%s': %s",
		arg, mu_errstring(err));
	  }
      break;


      /* log */
    case ARG_LOG_FACILITY:
      log_facility = parse_log_facility (arg);
      break;

      /* authentication */
#ifdef USE_LIBPAM
    case ARG_PAM_SERVICE:
      pam_service = arg;
      break;
#endif

#ifdef HAVE_MYSQL
    case ARG_SQL_GETPWNAM:
      sql_getpwnam_query = arg;
      break;

    case ARG_SQL_GETPWUID:
      sql_getpwuid_query = arg;
      break;

    case ARG_SQL_GETPASS:
      sql_getpass_query = arg;
      break;

    case ARG_SQL_HOST:
      sql_host = arg;
      break;

    case ARG_SQL_USER:
      sql_user = arg;
      break;

    case ARG_SQL_PASSWD:
      sql_passwd = arg;
      break;

    case ARG_SQL_DB:
      sql_db = arg;
      break;

    case ARG_SQL_PORT:
      sql_port = strtoul (arg, NULL, 0);
      if (sql_port == 0)
	{
	  sql_host = NULL;
	  sql_socket = arg;
	}
      break;

#endif
    case ARGP_KEY_FINI:
      p = mu_normalize_maildir (mu_path_maildir);
      if (!p)
	{
	  argp_error (state, "badly formed maildir: %s", mu_path_maildir);
	}
      mu_path_maildir = p;
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
      
    case 't':
      p->timeout = strtoul (arg, NULL, 10);
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
    {
      fprintf (stderr, "%s: not enough memory\n", progname);
      exit (1);
    }

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
	      fprintf (stderr, "%s: not enough memory\n", progname);
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
              
	  if (argcv_get (p, "", NULL, &n_argc, &n_argv))
	    {
	      argcv_free (n_argc, n_argv);
	      if (linebuf)
		free (linebuf);
	      linebuf = NULL;
	      continue;
	    }
	  x_argv = realloc (x_argv,
			    (x_argc + n_argc) * sizeof (x_argv[0]));
	  if (!x_argv)
	    {
	      fprintf (stderr, "%s: not enough memory\n", progname);
	      exit (1);
	    }
	  
	  for (i = 0; i < n_argc; i++)
	    x_argv[x_argc++] = n_argv[i];
	  
	  free (n_argv);
	  if (linebuf)
	    free (linebuf);
	  linebuf = NULL;
	}
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
      fprintf (stderr, "%s: not enough memory\n", progname);
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

    if (!rcdirname)
      {
	fprintf (stderr, "%s: not enough memory\n", progname);
	exit (1);
      }
    if(stat(rcdirname, &s) == 0 && S_ISDIR(s.st_mode))
      rcdir = 1;

    free(rcdirname);
  }

  /* Add per-user config file. */
  if(!rcdir)
  {
    read_rc (progname, MU_USER_CONFIG_FILE, capa, &x_argc, &x_argv);
  }
  else
  {
    char* userrc = NULL;

    mu_asprintf(&userrc, "%s/mailutils", MU_USER_CONFIG_FILE);

    if (!userrc)
      {
	fprintf (stderr, "%s: not enough memory\n", progname);
	exit (1);
      }
    read_rc (progname, userrc, capa, &x_argc, &x_argv);

    free(userrc);
  }

  /* Add per-user, per-program config file. */
  {
    char* progrc = NULL;

    if(rcdir)
      mu_asprintf(&progrc, "%s/%src", MU_USER_CONFIG_FILE, progname);
    else
      mu_asprintf(&progrc, "~/.mu.%src", progname);

    if (!progrc)
      {
	fprintf (stderr, "%s: not enough memory\n", progname);
	exit (1);
      }

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

struct argp_capa {
  char *capability;
  struct argp_child *child;
} mu_argp_capa[] = {
  {"common",  &mu_common_argp_child},
  {"license", &mu_license_argp_child},
  {"mailbox", &mu_mailbox_argp_child},
  {"address", &mu_address_argp_child},
  {"mailer",  &mu_mailer_argp_child},
  {"logging", &mu_logging_argp_child},
  {"auth",    &mu_auth_argp_child},
  {"daemon",  &mu_daemon_argp_child},
  {NULL,}
};

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
      mu_error ("out of memory");
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
	  mu_error ("INTERNAL ERROR: requested unknown argp capability %s",
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
      mu_error ("out of memory");
      abort ();
    }

  memcpy (argp, template, sizeof (*argp));

  argp->children = ap;

  return argp;
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

  if(!argp)
    argp = &argpnull;
  
  argp = mu_build_argp (argp, capa);
  mu_create_argcv (capa, *pargc, *pargv, pargc, pargv);
  ret = argp_parse (argp, *pargc, *pargv, flags, arg_index, input);
  free ((void*) argp->children);
  free ((void*) argp);
  return ret;
}

