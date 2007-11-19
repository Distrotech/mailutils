/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

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
#include "mailutils/libargp.h"
#include <string.h>
#include <mailutils/syslog.h>
#include <mailutils/daemon.h>

static struct mu_gocs_daemon daemon_settings;
static struct mu_gocs_locking locking_settings;
static struct mu_gocs_logging logging_settings;
static struct mu_gocs_mailbox mailbox_settings;
static struct mu_gocs_source_email source_email_settings;
static struct mu_gocs_mailer mailer_settings;


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


/* ************************************************************************* */
/* Common                                                                    */
/* ************************************************************************* */

static struct argp_option mu_common_argp_options[] = 
{
  { NULL, 0, NULL, 0, N_("Common options"), 0},
  { "show-config-options", OPT_SHOW_OPTIONS, NULL, OPTION_HIDDEN,
    N_("Show compilation options"), 0 },
  { "no-user-rcfile", OPT_NO_USER_RCFILE, NULL, 0,
    N_("Do not load user configuration file"), 0 },
  { "no-site-rcfile", OPT_NO_SITE_RCFILE, NULL, 0,
    N_("Do not load site configuration file"), 0 },
  { "rcfile", OPT_RCFILE, N_("FILE"), 0,
    N_("Load this configuration file"), 0, },
  { "rcfile-verbose", OPT_RCFILE_VERBOSE, NULL, 0,
    N_("Verbosely log parsing of the configuration files"), 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
mu_common_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case OPT_SHOW_OPTIONS:
      mu_print_options ();
      exit (0);

    case OPT_NO_USER_RCFILE:
      mu_load_user_rcfile = 0;
      break;
      
    case OPT_NO_SITE_RCFILE:
      mu_load_site_rcfile = 0;
      break;
      
    case OPT_RCFILE:
      mu_load_rcfile = arg;
      break;

    case OPT_RCFILE_VERBOSE:
      mu_cfg_parser_verbose++;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

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

struct mu_cmdline_capa mu_common_cmdline = {
  "common", &mu_common_argp_child
};


/* ************************************************************************* */
/* Logging                                                                   */
/* ************************************************************************* */

static struct argp_option mu_logging_argp_option[] = {
  {"log-facility", OPT_LOG_FACILITY, N_("FACILITY"), 0,
   N_("Output logs to syslog FACILITY"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_logging_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
      /* log */
    case OPT_LOG_FACILITY:
      if (mu_string_to_syslog_facility (arg, &logging_settings.facility))
	mu_error (_("Unknown syslog facility `%s'"), arg);
      /* FIXME: error reporting */
      break;

    case ARGP_KEY_FINI:
      mu_gocs_store ("logging", &logging_settings);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_logging_argp = {
  mu_logging_argp_option,
  mu_logging_argp_parser,
};

struct argp_child mu_logging_argp_child = {
  &mu_logging_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_logging_cmdline = {
  "logging", &mu_logging_argp_child
};


/* ************************************************************************* */
/* License                                                                   */
/* ************************************************************************* */

/* Option to print the license. */
static struct argp_option mu_license_argp_option[] = {
  { "license", OPT_LICENSE, NULL, 0, N_("Print license and exit"), -2 },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_license_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case OPT_LICENSE:
      printf (_("License for %s:\n\n"), argp_program_version);
      printf ("%s", mu_license_text);
      exit (0);

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_license_argp = {
  mu_license_argp_option,
  mu_license_argp_parser,
};

struct argp_child mu_license_argp_child = {
  &mu_license_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_license_cmdline = {
  "license", &mu_license_argp_child 
};


/* ************************************************************************* */
/* Mailbox                                                                   */
/* ************************************************************************* */

/* Options used by programs that access mailboxes. */
static struct argp_option mu_mailbox_argp_option[] = {
  { "mail-spool", 'm', N_("URL"), OPTION_HIDDEN,
    N_("Use specified URL as a mailspool directory"), 0 },
  { "mailbox-type", OPT_MAILBOX_TYPE, N_("PROTO"), OPTION_HIDDEN,
    N_("Default mailbox type to use"), 0 },
  { NULL }
};

static error_t
mu_mailbox_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
      /* mailbox */
    case 'm':
      assign_string (&mailbox_settings.mail_spool, arg);
      break;

    case OPT_MAILBOX_TYPE:
      assign_string (&mailbox_settings.mailbox_type, arg);
      break;
      
    case ARGP_KEY_FINI:
      mu_gocs_store ("mailbox", &mailbox_settings);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_mailbox_argp = {
  mu_mailbox_argp_option,
  mu_mailbox_argp_parser,
};

struct argp_child mu_mailbox_argp_child = {
  &mu_mailbox_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_mailbox_cmdline = {
  "mailbox", &mu_mailbox_argp_child
};


/* ************************************************************************* */
/* Locking                                                                   */
/* ************************************************************************* */

/* Options used by programs that access mailboxes. */
static struct argp_option mu_locking_argp_option[] = {
  {"lock-flags", OPT_LOCK_FLAGS, N_("FLAGS"), OPTION_HIDDEN,
   N_("Default locker flags (E=external, R=retry, T=time, P=pid)"), 0},
  {"lock-retry-timeout", OPT_LOCK_RETRY_TIMEOUT, N_("SECONDS"), OPTION_HIDDEN,
   N_("Set timeout for acquiring the lockfile") },
  {"lock-retry-count", OPT_LOCK_RETRY_COUNT, N_("NUMBER"), OPTION_HIDDEN,
   N_("Set the maximum number of times to retry acquiring the lockfile") },
  {"lock-expire-timeout", OPT_LOCK_EXPIRE_TIMEOUT, N_("SECONDS"),
   OPTION_HIDDEN,
   N_("Number of seconds after which the lock expires"), },
  {"external-locker", OPT_LOCK_EXTERNAL_PROGRAM, N_("PATH"), OPTION_HIDDEN,
   N_("Set full path to the external locker program") },
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_locking_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case OPT_LOCK_FLAGS:
      assign_string (&locking_settings.lock_flags, arg);
      break;

    case OPT_LOCK_RETRY_COUNT:
      locking_settings.lock_retry_count = strtoul (arg, NULL, 0);
      break;
	  
    case OPT_LOCK_RETRY_TIMEOUT:
      locking_settings.lock_retry_timeout = strtoul (arg, NULL, 0);
      break;

    case OPT_LOCK_EXPIRE_TIMEOUT:
      locking_settings.lock_expire_timeout = strtoul (arg, NULL, 0);
      break;

    case OPT_LOCK_EXTERNAL_PROGRAM:
      assign_string (&locking_settings.external_locker, arg);
      break;
      
    case ARGP_KEY_FINI:
      mu_gocs_store ("locking", &locking_settings);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_locking_argp = {
  mu_locking_argp_option,
  mu_locking_argp_parser,
};

struct argp_child mu_locking_argp_child = {
  &mu_locking_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_locking_cmdline = {
  "locking", &mu_locking_argp_child
};
  

/* ************************************************************************* */
/* Address                                                                   */
/* ************************************************************************* */

/* Options used by programs that do address mapping. */
static struct argp_option mu_address_argp_option[] = {
  {"email-addr", 'E', N_("EMAIL"), OPTION_HIDDEN,
   N_("Set current user's email address (default is loginname@defaultdomain)"), 0},
  {"email-domain", 'D', N_("DOMAIN"), OPTION_HIDDEN,
   N_("Set domain for unqualified user names (default is this host)"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_address_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'E':
      assign_string (&source_email_settings.address, arg);
      break;

    case 'D':
      assign_string (&source_email_settings.domain, arg);
      break;

    case ARGP_KEY_FINI:
      mu_gocs_store ("address", &source_email_settings);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}
  
struct argp mu_address_argp = {
  mu_address_argp_option,
  mu_address_argp_parser,
};

struct argp_child mu_address_argp_child = {
  &mu_address_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_address_cmdline = {
  "address", &mu_address_argp_child
};


/* ************************************************************************* */
/* Mailer                                                                    */
/* ************************************************************************* */

/* Options used by programs that send mail. */
static struct argp_option mu_mailer_argp_option[] = {
  {"mailer", 'M', N_("MAILER"), 0,
   N_("Use specified URL as the default mailer"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
mu_mailer_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
      /* mailer */
    case 'M':
      assign_string (&mailer_settings.mailer, arg);
      break;

    case ARGP_KEY_FINI:
      mu_gocs_store ("mailer", &mailer_settings);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp mu_mailer_argp = {
  mu_mailer_argp_option,
  mu_mailer_argp_parser,
};

struct argp_child mu_mailer_argp_child = {
  &mu_mailer_argp,
  0,
  NULL,
  0
};

struct mu_cmdline_capa mu_mailer_cmdline = {
  "mailer", &mu_mailer_argp_child
};


/* ************************************************************************* */
/* Daemon                                                                    */
/* ************************************************************************* */

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

static error_t
mu_daemon_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'd':
      daemon_settings.mode = MODE_DAEMON;
      if (arg)
	daemon_settings.maxchildren = strtoul (arg, NULL, 10);
      break;

    case 'i':
      daemon_settings.mode = MODE_INTERACTIVE;
      break;
      
    case 'p':
      daemon_settings.mode = MODE_DAEMON;
      daemon_settings.port = strtoul (arg, NULL, 10); /*FIXME: overflow */
      break;

    case 'P':
      assign_string (&daemon_settings.pidfile, arg);
      break;

    case 't':
      daemon_settings.timeout = strtoul (arg, NULL, 10);
      break;

    case 'x':
      daemon_settings.transcript = 1;
      break;

    case ARGP_KEY_FINI:
      mu_gocs_store ("daemon", &daemon_settings);
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

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

struct mu_cmdline_capa mu_daemon_cmdline = {
  "daemon", &mu_daemon_argp_child
};

