/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <mailutils/types.h>
#include <mailutils/nls.h>
#include <mailutils/io.h>
#include <mailutils/errno.h>
#include <mailutils/log.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/filter.h>
#include <mailutils/syslog.h>

int
mu_stdstream_strerr_create (mu_stream_t *plogger, int type, int facility,
			    int priority, const char *tag,
			    const char *fname MU_ARG_UNUSED)
{
  int rc;
  mu_stream_t transport;
  
  switch (type)
    {
    case MU_STRERR_STDERR:
      {
        int yes = 1;
	mu_stream_t str;
      
	rc = mu_stdio_stream_create (&str, MU_STDERR_FD, 0);
	if (rc)
	  {
	    fprintf (stderr, _("%s: cannot open error stream: %s\n"),
		     tag ? tag : "<unknown>", mu_strerror (rc));
	    return MU_ERR_FAILURE;
	  }
        /* Make sure 2 is not closed when str is destroyed.
           FIXME: Actually I don't know if it is needed, but just
                  in case... */
        mu_stream_ioctl (str, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
	if (!tag)
	  transport = str;
	else
	  {
	    char *fltargs[3] = { "INLINE-COMMENT", };
	    mu_asprintf (&fltargs[1], "%s: ", tag);
	    fltargs[2] = NULL;
	    rc = mu_filter_create_args (&transport, str,
					"INLINE-COMMENT",
					2, (const char**)fltargs,
					MU_FILTER_ENCODE, MU_STREAM_WRITE);
	    mu_stream_unref (str);
	    free (fltargs[1]);
	    if (rc)
	      {
		fprintf (stderr,
			 _("%s: cannot open output filter stream: %s"),
			 tag ? tag : "<unknown>", mu_strerror (rc));
		return MU_ERR_FAILURE;
	      }
	    mu_stream_set_buffer (transport, mu_buffer_line, 0);
	  }
      }
      break;

    case MU_STRERR_SYSLOG:
      openlog (tag, LOG_PID, facility);
      rc = mu_syslog_stream_create (&transport, priority);
      if (rc)
	{
	  fprintf (stderr, _("%s: cannot create syslog stream: %s\n"),
		   tag ? tag : "<unknown>", mu_strerror (rc));
	  return MU_ERR_FAILURE;
	}
      break;

    default:
      fprintf (stderr, _("%s: cannot create error stream: %s\n"),
	       tag ? tag : "<unknown>", mu_strerror (EINVAL));
      return EINVAL;
    }

  rc = mu_log_stream_create (plogger, transport);
  mu_stream_unref (transport);
  if (rc)
    {
      fprintf (stderr, _("%s: cannot open logger stream: %s\n"),
	       tag ? tag : "<unknown>", mu_strerror (rc));
      return MU_ERR_FAILURE;
    }
  return 0;
}
      
int
mu_stdstream_strerr_setup (int type)
{
  mu_stream_t str;

  if (mu_stdstream_strerr_create (&str, type,
				  mu_log_facility, LOG_ERR,
				  mu_log_tag, NULL) == 0)
    {
      if (mu_log_print_severity)
	{
	  int mode = MU_LOGMODE_SEVERITY;
	  mu_stream_ioctl (str, MU_IOCTL_LOGSTREAM, 
                           MU_IOCTL_LOGSTREAM_SET_MODE, &mode);
	}
      if (mu_log_severity_threshold > MU_LOG_DEBUG)
	mu_stream_ioctl (str, MU_IOCTL_LOGSTREAM,
                         MU_IOCTL_LOGSTREAM_SUPPRESS_SEVERITY,
			 &mu_log_severity_threshold);
      mu_stream_destroy (&mu_strerr);
      mu_strerr = str;
    }
  return 0;
}
