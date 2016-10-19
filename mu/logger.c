/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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

#include "mu.h"

char logger_docstring[] = N_("log data using Mailutils log facility");
static char logger_args_doc[] = N_("[TEXT]");

static char *input_file = NULL;
static int logger_type = MU_STRERR_STDERR;
static int log_severity = MU_LOG_ERROR;
static struct mu_locus locus;
static int syslog_facility = LOG_USER;
static int syslog_priority = LOG_ERR;
static char *syslog_tag = NULL;

static void
set_priority (struct mu_parseopt *po, struct mu_option *opt,
	      char const *arg)
{
  char *s = strchr (arg, '.');
  if (s)
    *s++ = 0;
  if (mu_string_to_syslog_facility (arg, &syslog_facility))
    {
      mu_parseopt_error (po, _("unknown facility: %s"), arg);
      exit (po->po_exit_error);
    }
  if (s && mu_string_to_syslog_priority (s, &syslog_priority))
    {
      mu_parseopt_error (po, _("unknown priority: %s"), s);
      exit (po->po_exit_error);
    }
  
  logger_type = MU_STRERR_SYSLOG;
}

static void
set_syslog (struct mu_parseopt *po, struct mu_option *opt,
	    char const *arg)
{
  logger_type = MU_STRERR_SYSLOG;
}

static void
set_stderr (struct mu_parseopt *po, struct mu_option *opt,
	    char const *arg)
{
  logger_type = MU_STRERR_STDERR;
}

static void
set_severity (struct mu_parseopt *po, struct mu_option *opt,
	      char const *arg)
{
  int i;
  
  for (i = 0; i < _mu_severity_num; i++)
    if (mu_c_strcasecmp (_mu_severity_str[i], arg) == 0)
      {
	log_severity = i;
	return;
      }
  mu_parseopt_error (po, _("unknown severity: %s"), arg);
  exit (po->po_exit_error);
}

static void
set_locus (struct mu_parseopt *po, struct mu_option *opt,
	   char const *arg)
{
  char *s;
  
  locus.mu_file = mu_strdup (arg);
  s = strchr (arg, ':');
  if (s)
    {
      *s++ = 0;
      locus.mu_line = strtoul (s, &s, 10);
      if (*s == ':')
	{
	  locus.mu_col = strtoul (s + 1, &s, 10);
	  if (*s)
	    {
	      mu_parseopt_error (po, _("bad column number: %s"), arg);
	      exit (po->po_exit_error);
	    }
	}
      else if (*s)
	{
	  mu_parseopt_error (po, _("bad line number: %s"), arg);
	  exit (po->po_exit_error);
	}
    }
}

static struct mu_option logger_options[] = {
  { "file", 'f', N_("FILE"), MU_OPTION_DEFAULT,
    N_("read message from FILE"),
    mu_c_string, &input_file },
  { "priority", 'p', N_("FACILITY[.LEVEL]"), MU_OPTION_DEFAULT,
    N_("log at the specified syslog priority (implies --syslog)"),
    mu_c_string, NULL, set_priority },
  { "syslog", 0, NULL, MU_OPTION_DEFAULT,
    N_("log via syslog"),
    mu_c_string, NULL, set_syslog },
  { "stderr", 0, NULL, MU_OPTION_DEFAULT,
    N_("log to the standard error"),
    mu_c_string, NULL, set_stderr },  
  { "severity", 's', N_("SEV"), MU_OPTION_DEFAULT,
    N_("log at Mailutils severity level SEV"),
    mu_c_string, NULL, set_severity },
  { "locus", 'l', N_("FILE:LINE[:COL]"), MU_OPTION_DEFAULT,
    N_("set locus for logging"),
    mu_c_string, NULL, set_locus },
  { "tag", 't', N_("TAG"), MU_OPTION_DEFAULT,
    N_("set syslog tag"),
    mu_c_string, &syslog_tag },
  MU_OPTION_END
};

int
mutool_logger (int argc, char **argv)
{
  mu_stream_t logger, input;
  int rc, mode;

  mu_action_getopt (&argc, &argv, logger_options, logger_docstring,
		    logger_args_doc);

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

  
  
