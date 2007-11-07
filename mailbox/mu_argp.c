/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2003, 2004, 2005, 2006,
   2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

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
#include <mailutils/cfg.h>


/* ************************************************************************* */
/* Variables and auxiliary functions.                                        */
/* FIXME: The variables should be global.                                    */
/* ************************************************************************* */

static char *mail_spool_option;
static char *mailbox_type_option;
static char *lock_flags_option;
static unsigned long lock_retry_timeout_option;
static unsigned long lock_retry_count_option;
static unsigned long lock_expire_timeout_option;
static char *external_locker_option;

static char *email_addr_option;
static char *email_domain_option;

static char *mailer_option;

static char *log_facility_option;

static unsigned int daemon_max_children_option;
static char *daemon_mode_option;
static unsigned int daemon_transcript_option;
static char *daemon_pidfile_option;
static unsigned short daemon_port_option;
static unsigned int daemon_timeout_option;

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
    "   the Free Software Foundation; either version 3 of the License, or\n"
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

void
assign_string (char **pstr, char *val)
{
  if (!val)
    {
      if (*pstr)
	{
	  free (*pstr);
	  *pstr = NULL;
	}
    }
  else
    {
      size_t size = strlen (val);
      char *p = realloc (*pstr, size + 1);
      if (!p)
	{
	  mu_error ("%s", mu_strerror (ENOMEM));
	  exit (1);
	}
      strcpy (p, val);
      *pstr = p;
    }
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


/* ************************************************************************* */
/* Traditional (command-line style) configuration                            */
/* ************************************************************************* */

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
  {"mail-spool", 'm', N_("URL"), OPTION_HIDDEN,
   N_("Use specified URL as a mailspool directory"), 0},
  {"mailbox-type", ARG_MAILBOX_TYPE, N_("PROTO"), OPTION_HIDDEN,
   N_("Default mailbox type to use"), 0 },
  {"lock-flags", ARG_LOCK_FLAGS, N_("FLAGS"), OPTION_HIDDEN,
   N_("Default locker flags (E=external, R=retry, T=time, P=pid)"), 0},
  {"lock-retry-timeout", ARG_LOCK_RETRY_TIMEOUT, N_("SECONDS"), OPTION_HIDDEN,
   N_("Set timeout for acquiring the lockfile") },
  {"lock-retry-count", ARG_LOCK_RETRY_COUNT, N_("NUMBER"), OPTION_HIDDEN,
   N_("Set the maximum number of times to retry acquiring the lockfile") },
  {"lock-expire-timeout", ARG_LOCK_EXPIRE_TIMEOUT, N_("SECONDS"), OPTION_HIDDEN,
   N_("Number of seconds after which the lock expires"), },
  {"external-locker", ARG_LOCK_EXTERNAL_PROGRAM, N_("PATH"), OPTION_HIDDEN,
   N_("Set full path to the external locker program") },
  { NULL,      0, NULL, 0, NULL, 0 }
};

/* Options used by programs that do address mapping. */
static struct argp_option mu_address_argp_option[] = {
  {"email-addr", 'E', N_("EMAIL"), OPTION_HIDDEN,
   N_("Set current user's email address (default is loginname@defaultdomain)"), 0},
  {"email-domain", 'D', N_("DOMAIN"), OPTION_HIDDEN,
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
  {"timeout", 't', N_("NUMBER"), OPTION_HIDDEN,
   N_("Set idle timeout value to NUMBER seconds"), 0},
  {"transcript", 'x', NULL, 0,
   N_("Output session transcript via syslog"), 0},
  {"pidfile", 'P', N_("FILE"), OPTION_HIDDEN,
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
      assign_string (&mail_spool_option, arg);
      break;

    case ARG_MAILBOX_TYPE:
      assign_string (&mailbox_type_option, arg);
      break;
      
    case ARG_LOCK_FLAGS:
      assign_string (&lock_flags_option, arg);
      break;

    case ARG_LOCK_RETRY_COUNT:
      lock_retry_count_option = strtoul (arg, NULL, 0);
      break;
	  
    case ARG_LOCK_RETRY_TIMEOUT:
      lock_retry_timeout_option = strtoul (arg, NULL, 0);
      break;

    case ARG_LOCK_EXPIRE_TIMEOUT:
      lock_expire_timeout_option = strtoul (arg, NULL, 0);
      break;

    case ARG_LOCK_EXTERNAL_PROGRAM:
      assign_string (&external_locker_option, arg);
      break;
      
      /* address */
    case 'E':
      assign_string (&email_addr_option, arg);
      break;

    case 'D':
      assign_string (&email_domain_option, arg);
      break;

      /* mailer */
    case 'M':
      assign_string (&mailer_option, arg);
      break;

      /* log */
    case ARG_LOG_FACILITY:
      assign_string (&log_facility_option, arg);
      break;

    case ARGP_KEY_FINI:
      if (mail_spool_option)
	{
	  err = mu_set_mail_directory (mail_spool_option);
	  if (err)
	    argp_error (state, _("Cannot set mail directory name: %s"),
			mu_strerror (err));
	  free (mail_spool_option);
	  mail_spool_option = NULL;
	}

      if (mailbox_type_option)
	{
	  if (mu_mailbox_set_default_proto (mailbox_type_option))
	    argp_error (state, _("Invalid mailbox type: %s"),
			mailbox_type_option);
	  free (mailbox_type_option);
	  mailbox_type_option = NULL;
	}

      if (lock_flags_option)
	{
	  int flags = 0;
	  char *p;
	  
	  for (p = lock_flags_option; *p; p++)
	    {
	      switch (*p)
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
		  argp_error (state, _("Invalid lock flag `%c'"), *p);
		}
	    }
	  mu_locker_set_default_flags (flags, mu_locker_assign);
	  free (lock_flags_option);
	  lock_flags_option = NULL;
	}

      if (lock_retry_count_option)
	{
	  mu_locker_set_default_retry_count (lock_retry_count_option);
	  mu_locker_set_default_flags (MU_LOCKER_RETRY, mu_locker_set_bit);
	  lock_retry_count_option = 0;
	}

      if (lock_retry_timeout_option)
	{
	  mu_locker_set_default_retry_timeout (lock_retry_timeout_option);
	  mu_locker_set_default_flags (MU_LOCKER_RETRY, mu_locker_set_bit);
	  lock_retry_timeout_option = 0;
	}

      if (lock_expire_timeout_option)
	{
	  mu_locker_set_default_expire_timeout (lock_expire_timeout_option);
	  mu_locker_set_default_flags (MU_LOCKER_EXTERNAL, mu_locker_set_bit);
	  lock_expire_timeout_option = 0;
	}

      if (external_locker_option)
	{
	  mu_locker_set_default_external_program (external_locker_option);
	  mu_locker_set_default_flags (MU_LOCKER_TIME, mu_locker_set_bit);
	  free (external_locker_option);
	  external_locker_option = NULL;
	}

      if (email_addr_option)
	{
	  if ((err = mu_set_user_email (email_addr_option)) != 0)
	    {
	      argp_error (state, _("Invalid email address `%s': %s"),
			  email_addr_option, mu_strerror (err));
	    }
	  free (email_addr_option);
	  email_addr_option = NULL;
	}

      if (email_domain_option)
	{
	  if ((err = mu_set_user_email_domain (email_domain_option)) != 0)
	    {
	      argp_error (state, _("Invalid email domain `%s': %s"),
			  email_domain_option, mu_strerror (err));
	    }
	  free (email_domain_option);
	  email_domain_option = NULL;
	}

      if (mailer_option)
	{
	  if ((err = mu_mailer_set_url_default (mailer_option)) != 0)
	    {
	      argp_error (state, _("Invalid mailer URL `%s': %s"),
			  mailer_option, mu_strerror (err));
	    }
	  free (mailer_option);
	  mailer_option = NULL;
	}

      if (log_facility_option)
	{
	  log_facility = parse_log_facility (log_facility_option);
	  free (log_facility_option);
	  log_facility_option = NULL;
	}
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
      assign_string (&daemon_mode_option, "daemon");
      if (arg)
        {
          unsigned long n = strtoul (arg, NULL, 10);
          if (n > 0)
            daemon_max_children_option = n;
        }
      break;

    case 'i':
      assign_string (&daemon_mode_option, "interactive");
      break;
      
    case 'p':
      assign_string (&daemon_mode_option, "daemon");
      daemon_port_option = strtoul (arg, NULL, 10); /*FIXME: overflow */
      break;

    case 'P':
      assign_string (&daemon_pidfile_option, arg);
      break;

    case 't':
      daemon_timeout_option = strtoul (arg, NULL, 10);
      break;

    case 'x':
      daemon_transcript_option = 1;
      break;

    case ARGP_KEY_FINI:
      if (daemon_mode_option)
	{
	  if (strcmp (daemon_mode_option, "daemon") == 0)
	    p->mode = MODE_DAEMON;
	  else if (strcmp (daemon_mode_option, "interactive") == 0)
	    p->mode = MODE_INTERACTIVE;
	  else /* FIXME */
	    argp_error (state, _("Invalid daemon mode: `%s'"),
			daemon_mode_option);
	  free (daemon_mode_option);
	  daemon_mode_option = 0;
	}

      if (daemon_max_children_option)
	{
	  p->mode = MODE_DAEMON;
	  p->maxchildren = daemon_max_children_option;
	  daemon_max_children_option = 0;
	}

      if (daemon_pidfile_option)
	{
	  p->mode = MODE_DAEMON;
	  p->pidfile = daemon_pidfile_option;
	  daemon_pidfile_option = NULL;
	}

      p->transcript = daemon_transcript_option;
      if (daemon_timeout_option)
	{
	  p->timeout = daemon_timeout_option;
	  daemon_timeout_option = 0;
	}
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}


/* ************************************************************************* */
/* New (resource-file style) configuration                                   */
/* ************************************************************************* */

static struct mu_cfg_param mu_mailbox_param[] = {
  { "mail-spool", mu_cfg_string, &mail_spool_option },
  { "mailbox-type", mu_cfg_string, &mailbox_type_option },
  { "lock-flags", mu_cfg_string, &lock_flags_option },
  { "lock-retry-timeout", mu_cfg_ulong, &lock_retry_timeout_option },
  { "lock-retry-count", mu_cfg_ulong, &lock_retry_count_option },
  { "lock-expire-timeout", mu_cfg_ulong, &lock_expire_timeout_option },
  { "external-locker", mu_cfg_string, &external_locker_option },
  { NULL, }
};

static struct mu_cfg_param mu_address_param[] = {
  { "email-addr", mu_cfg_string, &email_addr_option },
  { "email-domain", mu_cfg_string, &email_domain_option },
  { NULL }
};

static struct mu_cfg_param mu_mailer_param[] = {
  { "url", mu_cfg_string, &mailer_option },
  { NULL }
};

static struct mu_cfg_param mu_logging_param[] = {
  { "facility", mu_cfg_string, &log_facility_option },
  { NULL }
};

static struct mu_cfg_param mu_daemon_param[] = {
  { "max-children", mu_cfg_ulong, &daemon_max_children_option },
  { "mode", mu_cfg_string, &daemon_mode_option },
  { "transcript", mu_cfg_bool, &daemon_transcript_option },
  { "pidfile", mu_cfg_string, &daemon_pidfile_option },
  { "port", mu_cfg_ushort, &daemon_port_option },
  { "timeout", mu_cfg_ulong, &daemon_timeout_option },
  { NULL }
};


/* ************************************************************************* */
/* Capability array and auxiliary functions.                                 */
/* ************************************************************************* */

#define MU_MAX_CAPA 24

struct argp_capa {
  char *capability;
  struct argp_child *child;
  struct mu_cfg_param *param;
} mu_argp_capa[MU_MAX_CAPA] = {
  {"common",  &mu_common_argp_child},
  {"license", &mu_license_argp_child},
  {"mailbox", &mu_mailbox_argp_child, mu_mailbox_param },
  {"address", &mu_address_argp_child, mu_address_param},
  {"mailer",  &mu_mailer_argp_child, mu_mailer_param },
  {"logging", &mu_logging_argp_child, mu_logging_param },
  {"daemon",  &mu_daemon_argp_child, mu_daemon_param },
  {NULL,}
};

int
mu_register_capa (const char *name, struct argp_child *child,
		  struct mu_cfg_param *param)
{
  int i;
  
  for (i = 0; i < MU_MAX_CAPA; i++)
    if (mu_argp_capa[i].capability == NULL)
      {
	mu_argp_capa[i].capability = strdup (name);
	mu_argp_capa[i].child = child;
	mu_argp_capa[i].param = param;
	return 0;
      }
  return 1;
}

static struct argp_capa *
find_capa (const char *name)
{
  int i;
  for (i = 0; mu_argp_capa[i].capability; i++)
    if (strcmp (mu_argp_capa[i].capability, name) == 0)
      return &mu_argp_capa[i];
  return NULL;
}

void
build_config_struct (const char *capa[])
{
  struct argp_capa *acp;
  
  for (acp = mu_argp_capa; acp->capability; acp++)
    {
      struct mu_cfg_param *param;
      if (member (capa, acp->capability, strlen (acp->capability)))
	param = acp->param;
      else
	param = NULL;
      mu_config_register_plain_section (NULL, acp->capability, param);
    }
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
      struct argp_capa *cp = find_capa (capa[n]);
      if (!cp)
	{
	  mu_error (_("INTERNAL ERROR: requested unknown argp "
		      "capability %s (please report)"),
		    capa[n]);
	  abort ();
	}
      ap[nchild] = *cp->child;
      ap[nchild].group = group++;
      nchild++;

    }
  ap[nchild].argp = NULL;

  build_config_struct (capa);

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


/* ************************************************************************* */
/* Configuration style detection                                             */
/* ************************************************************************* */

#define CF_GLOBAL 0
#define CF_USER   1
#define CF_SIZE   2

enum mu_config_flavor
  {
    mu_config_none,
    mu_config_auto,
    mu_config_options,
    mu_config_resource
  };

#define isword(c) (isalnum (c) || (c) == '-' || (c) == '_')

static int
getword (FILE *fp, int c)
{
  if (c == EOF || isspace (c) || c == '\\')
    return 0;
  if (c == '"' || c == '\'')
    {
      int delim = c;
      while ((c = getc (fp)) != EOF && c != delim)
	{
	  if (c == '\\')
	    if ((c = getc (fp)) == EOF)
	      break;
	}
    }
  else
    while ((c = getc (fp)) != EOF && !isspace (c))
      ;
  return 1;
}

static enum mu_config_flavor
deduce_config_flavor (const char *file)
{
  int c;
  FILE *fp;
  size_t optcnt = 0;
  size_t wordcnt = 0;
  char *file_name = mu_tilde_expansion (file, "/", NULL);
  
  fp = fopen (file_name, "r");
  free (file_name);
  if (!fp)
    return mu_config_none;

  while (!feof (fp))
    {
      while ((c = getc (fp)) != EOF && isspace (c))
	;
      if (c == '-')
	{
	  if ((c = getc (fp)) == EOF)
	    break;
	  if (c == '-')
	    {
	      if ((c = getc (fp)) == EOF)
		break;
	    }
	  if (isword (c))
	    optcnt++;
	  if (getword (fp, c))
	    wordcnt++;
	}
      else if (getword (fp, c))
	wordcnt++;
    }
  fclose (fp);
  if (wordcnt == 0)
    return mu_config_none;
  else if (optcnt == 0)
    return mu_config_resource;
  else if (wordcnt / optcnt <= 2)
    return mu_config_options;
  return mu_config_resource;
}

static enum mu_config_flavor
decode_config_flavor (const char *str)
{
  if (strcmp (str, "none") == 0)
    return mu_config_none;
  else if (strcmp (str, "auto") == 0)
    return mu_config_auto;
  else if (strcmp (str, "resource") == 0
	   || strcmp (str, "new") == 0)
    return mu_config_resource;
  else if (strcmp (str, "options") == 0
	   || strcmp (str, "old") == 0)
    return mu_config_options;
  else
    mu_error (_("invalid configuration flavor: %s"), str);
  return mu_config_auto;
}

static void
get_default_config_flavor (const char *progname, enum mu_config_flavor cfl[])
{
  char *val = getenv ("MU_CONFIG_FLAVOR");

  cfl[CF_GLOBAL] = cfl[CF_USER] = mu_config_auto;
  if (val)
    {
      int i;
      int argc;
      char **argv;
      size_t proglen;

      mu_argcv_get (val, ":", NULL, &argc, &argv);

      proglen = strlen (progname);
      for (i = 0; i < argc; i++)
	{
	  char *p = strchr (argv[i], '=');
	  if (p)
	    {
	      size_t len = p - argv[i];
	      if (len == proglen &&
		  memcmp (progname, argv[i], len) == 0)
		cfl[CF_USER] = decode_config_flavor (p + 1);
	    }
	  else
	    cfl[CF_GLOBAL] = decode_config_flavor (argv[i]);
	}
      mu_argcv_free (argc, argv);
    }
  if (cfl[CF_USER] == mu_config_auto)
    cfl[CF_USER] = cfl[CF_GLOBAL];
}


/* ************************************************************************* */
/* Functions for handling traditional-style configuration files.             */
/* ************************************************************************* */

/* Appends applicable options found in file NAME to argv. If progname
   is NULL, all the options found are assumed to apply. Otherwise they
   apply only if the line starts with ":something", and something is
   found in the CAPA array, or the line starts with PROGNAME. */
void
read_rc (const char *progname, const char *name, enum mu_config_flavor rf,
	 const char *capa[],
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
      free (rcfile);
      return;
    }

  if (rf == mu_config_auto)
    mu_error (_("Notice: reading options from `%s'"), rcfile);
  
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
      if (progname)
	{
	  for (p = kwp; *p && !isspace (*p); p++)
	    len++;
	}
      else
	p = kwp; /* Use the whole line. */

      if (progname == NULL
	  || (kwp[0] == ':' && member (capa, kwp+1, len-1))
	  || strncmp (progname, kwp, len) == 0)
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
	  x_argv = realloc (x_argv, (x_argc + n_argc) * sizeof (x_argv[0]));
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
  free (rcfile);

  *argc = x_argc;
  *argv = x_argv;
}

void
mu_create_argcv (const char *capa[],
		 const char *progname,
		 enum mu_config_flavor cfl[CF_SIZE],
		 int argc, char **argv, int *p_argc, char ***p_argv)
{
  int x_argc;
  char **x_argv;
  int i;
  int rcdir = 0;

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
  if (cfl[CF_GLOBAL] == mu_config_options || cfl[CF_GLOBAL] == mu_config_auto)
    read_rc (progname, MU_CONFIG_FILE, cfl[CF_GLOBAL], capa, &x_argc, &x_argv);

  if (cfl[CF_USER] == mu_config_options || cfl[CF_USER] == mu_config_auto)
    {
      /* Look for per-user config files in ~/.mailutils/ or in ~/, but
	 not both. This allows mailutils' utilities to have their config
	 files segregated, if necessary. */
      struct stat s;
      char *rcdirname = mu_tilde_expansion (MU_USER_CONFIG_FILE, "/", NULL);

      if (!rcdirname
	  || (stat (rcdirname, &s) == 0 && S_ISDIR (s.st_mode)))
	rcdir = 1;
      
      free (rcdirname);

      /* Add per-user config file. */
      if (!rcdir)
	read_rc (progname, MU_USER_CONFIG_FILE, cfl[CF_USER], capa,
		 &x_argc, &x_argv);
      else
	{
	  char *userrc = NULL;

	  userrc = malloc (sizeof (MU_USER_CONFIG_FILE) 
			   /* provides an extra slot
			   for null byte as well */
			   + 1 /* slash */
			   + 9 /* mailutils */); 

	  if (!userrc)
	    {
	      mu_error (_("%s: not enough memory"), progname);
	      exit (1);
	    }
      
	  sprintf (userrc, "%s/mailutils", MU_USER_CONFIG_FILE);
	  read_rc (progname, userrc, cfl[CF_USER], capa, &x_argc, &x_argv);
	  
	  free (userrc);
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
	    mu_error (_("%s: not enough memory"), progname);
	    exit (1);
	  }

	if (rcdir)
	  sprintf (progrc, "%s/%src", MU_USER_CONFIG_FILE, progname);
	else
	  sprintf (progrc, "~/.mu.%src", progname);
	
	read_rc (NULL, progrc, cfl[CF_USER], capa, &x_argc, &x_argv);
	free (progrc);
      }
    }

  /* Finally, add the command line options */
  x_argv = realloc (x_argv, (x_argc + argc) * sizeof (x_argv[0]));
  for (i = 1; i < argc; i++)
    x_argv[x_argc++] = argv[i];

  x_argv[x_argc] = NULL;

  *p_argc = x_argc;
  *p_argv = x_argv;
}


/* ************************************************************************* */

static struct mu_cfg_param *prog_param;

void
mu_argp_set_config_param (struct mu_cfg_param *p)
{
  prog_param = p;
}

static void
read_configs (char *progname, enum mu_config_flavor cfl[CF_SIZE])
{
  size_t size;
  char *file_name;
  enum mu_config_flavor f; 

  if (cfl[CF_GLOBAL] != mu_config_none)
    {
      f = deduce_config_flavor (MU_CONFIG_FILE);
      if (f == mu_config_resource)
	{
	  switch (cfl[CF_GLOBAL])
	    {
	    case mu_config_auto:
	    case mu_config_resource:
	      mu_parse_config (MU_CONFIG_FILE, progname, prog_param, 1);
	      break;

	    default:
	      break;
	    }
	  cfl[CF_GLOBAL] = mu_config_none; /* Avoid reading the file by
					      read_rc */
	}
      else if (cfl[CF_GLOBAL] != mu_config_auto && cfl[CF_GLOBAL] != f)
	cfl[CF_GLOBAL] = mu_config_none;
    }

  if (cfl[CF_USER] == mu_config_auto || cfl[CF_USER] == mu_config_resource)
    {
      size = 3 + strlen (progname) + 1;
      file_name = malloc (size);
      if (file_name)
	{
	  strcpy (file_name, "~/.");
	  strcat (file_name, progname);

	  mu_parse_config (file_name, progname, prog_param, 0);

	  free (file_name);
	}
    }
}

void
mu_argp_init (const char *vers, const char *bugaddr)
{
  argp_program_version = vers ? vers : PACKAGE_STRING;
  argp_program_bug_address = bugaddr ? bugaddr : "<" PACKAGE_BUGREPORT ">";
}

error_t
mu_argp_parse (const struct argp *argp, 
	       int *pargc, char **pargv[],  
	       unsigned flags,
	       const char *capa[],
	       int *arg_index,     
	       void *input)
{
  error_t ret;
  const struct argp argpnull = { 0 };
  char *progname;
  enum mu_config_flavor cfl[CF_SIZE];
  
  /* Make sure we have program version and bug address initialized */
  mu_argp_init (argp_program_version, argp_program_bug_address);

  progname = strrchr ((*pargv)[0], '/');
  if (progname)
    progname++;
  else
    progname = (*pargv)[0];

  if (strlen (progname) > 3 && memcmp (progname, "lt-", 3) == 0)
    progname += 3;

  get_default_config_flavor (progname, cfl);
  
  if (!argp)
    argp = &argpnull;

  argp = mu_build_argp (argp, capa);

  read_configs (progname, cfl);
  
  mu_create_argcv (capa, progname, cfl, *pargc, *pargv, pargc, pargv);
  ret = argp_parse (argp, *pargc, *pargv, flags, arg_index, input);
  free ((void*) argp->children);
  free ((void*) argp);
  return ret;
}
