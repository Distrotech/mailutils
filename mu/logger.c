/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <mailutils/mailutils.h>
#include "argp.h"
#include "mu.h"

static char logger_doc[] = N_("mu logger - log data using Mailutils log facility");
char logger_docstring[] = N_("log data using Mailutils log facility");
static char logger_args_doc[] = N_("[TEXT]");

#define OPT_SYSLOG 256
#define OPT_STDERR 257

static struct argp_option logger_options[] = {
  { "file", 'f', N_("FILE"), 0, N_("read message from FILE") },
  { "priority", 'p', N_("FACILITY[.LEVEL]"), 0,
    N_("log at the specified syslog priority (implies --syslog)") },
  { "syslog", OPT_SYSLOG, NULL, 0, N_("log via syslog") },
  { "stderr", OPT_STDERR, NULL, 0, N_("log to the standard error") },
  { "severity", 's', N_("SEV"), 0,
    N_("log at Mailutils severity level SEV") },
  { "locus", 'l', N_("FILE:LINE[:COL]"), 0,
    N_("set locus for logging") },
  { "tag", 't', N_("syslog tag"), 0,
    N_("set syslog tag") },
  { NULL }
};

static char *input_file = NULL;
static int logger_type = MU_STRERR_STDERR;
static int log_severity = MU_LOG_ERROR;
static struct mu_locus locus;
static int syslog_facility = LOG_USER;
static int syslog_priority = LOG_ERR;
static char *syslog_tag = NULL;

static error_t
logger_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      input_file = arg;
      break;

    case OPT_SYSLOG:
      logger_type = MU_STRERR_SYSLOG;
      break;

    case OPT_STDERR:
      logger_type = MU_STRERR_STDERR;
      break;

    case 's':
      {
	int i;

	for (i = 0; i < _mu_severity_num; i++)
	  if (mu_c_strcasecmp (_mu_severity_str[i], arg) == 0)
	    {
	      log_severity = i;
	      return 0;
	    }
	argp_error (state, _("unknown severity: %s"), arg);
	break;
      }

    case 't':
      syslog_tag = arg;
      break;
      
    case 'p':
      {
	char *s = strchr (arg, '.');
	if (s)
	  *s++ = 0;
	if (mu_string_to_syslog_facility (arg, &syslog_facility))
	  argp_error (state, _("unknown facility: %s"), arg);
	if (s &&
	    mu_string_to_syslog_priority (s, &syslog_priority))
	  argp_error (state, _("unknown priority: %s"), s);
	logger_type = MU_STRERR_SYSLOG;
	break;
      }
      
    case 'l':
      {
	char *s;
	
	locus.mu_file = arg;
	s = strchr (arg, ':');
	if (s)
	  {
	    *s++ = 0;
	    locus.mu_line = strtoul (s, &s, 10);
	    if (*s == ':')
	      {
		locus.mu_col = strtoul (s + 1, &s, 10);
		if (*s)
		  argp_error (state, _("bad column number: %s"), arg);
	      }
	    else if (*s)
	      argp_error (state, _("bad line number: %s"), arg);
	  }
	break;
      }
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp logger_argp = {
  logger_options,
  logger_parse_opt,
  logger_args_doc,
  logger_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_logger (int argc, char **argv)
{
  int index;
  mu_stream_t logger, input;
  int rc, mode;
  
  if (argp_parse (&logger_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;
  argc -= index;
  argv += index;

  if (argc && input_file)
    {
      mu_error (_("both input file and message text given"));
      exit (1);
    }

  if (!syslog_tag)
    syslog_tag = "mu-logger";
  rc = mu_stdstream_strerr_create (&logger, logger_type,
				   syslog_facility, syslog_priority,
				   syslog_tag, NULL);
  if (rc)
    {
      mu_error (_("cannot create log stream: %s"),
		mu_strerror (rc));
      exit (1);
    }

  mode = MU_LOGMODE_SEVERITY | MU_LOGMODE_LOCUS;
  mu_stream_ioctl (logger, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_MODE, &mode);
  if (locus.mu_file)
    mu_stream_ioctl (logger, MU_IOCTL_LOGSTREAM,
                     MU_IOCTL_LOGSTREAM_SET_LOCUS, &locus);
  mu_stream_ioctl (logger, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_SEVERITY, &log_severity);
  
  if (argc)
    {
      int i;
      
      for (i = 0; i < argc; i++)
	{
	  if (i > 0)
	    mu_stream_write (logger, " ", 1, NULL);
	  mu_stream_write (logger, argv[i], strlen (argv[i]), NULL);
	}
      mu_stream_write (logger, "\n", 1, NULL);
      return 0;
    }
  else if (!input_file || strcmp (input_file, "-") == 0)
    {
      mu_stream_ref (mu_strin);
      input = mu_strin;
    }
  else
    {
      rc = mu_file_stream_create (&input, input_file, MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("cannot open input stream %s: %s"),
		    input_file, mu_strerror (rc));
	  return 1;
	}
    } 

  rc = mu_stream_copy (logger, input, 0, NULL);
  mu_stream_unref (input);
  mu_stream_unref (logger);
  return !!rc;
}

/*
  MU Setup: logger
  mu-handler: mutool_logger
  mu-docstring: logger_docstring
  End MU Setup:
*/

  
  
