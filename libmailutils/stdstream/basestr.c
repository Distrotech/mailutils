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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/log.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/util.h>
#include <mailutils/io.h>
#include <mailutils/filter.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/file_stream.h>
#include <mailutils/sys/logstream.h>

static void stdstream_flushall_setup (void);

/* This event callback bootstraps standard I/O streams mu_strin and
   mu_strout. It is invoked when the stream core emits the bootstrap
   event for the stream. */
static void
std_bootstrap (struct _mu_stream *str, int code,
	       unsigned long lval, void *pval)
{
  struct _mu_file_stream *fstr = (struct _mu_file_stream *) str;
  _mu_file_stream_setup (fstr);
  str->event_cb = NULL;
  str->event_mask = 0;
  str->event_cb_data = 0;
  fstr->stream.flags |= _MU_STR_OPEN;
  mu_stream_set_buffer ((mu_stream_t) fstr, mu_buffer_line, 0);
  stdstream_flushall_setup ();
}

/* This event callback bootstraps standard error stream (mu_strerr).
   It is invoked when the stream core emits the bootstrap event for
   the stream. */
static void
std_log_bootstrap (struct _mu_stream *str, int code,
		   unsigned long lval, void *pval)
{
  struct _mu_log_stream *logstr = (struct _mu_log_stream *) str;
  int yes = 1;
  mu_stream_t errstr, transport;
  int rc;
  
  rc = mu_stdio_stream_create (&errstr, MU_STDERR_FD, 0);
  if (rc)
    {
      fprintf (stderr, "%s: cannot open error stream: %s\n",
	       mu_program_name ? mu_program_name : "<unknown>",
	       mu_strerror (rc));
      abort ();
    }
  /* Make sure 2 is not closed when errstr is destroyed. */
  mu_stream_ioctl (errstr, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
  if (!mu_program_name)
    transport = errstr;
  else
    {
      char *fltargs[3] = { "INLINE-COMMENT", };
      mu_asprintf (&fltargs[1], "%s: ", mu_program_name);
      fltargs[2] = NULL;
      rc = mu_filter_create_args (&transport, errstr,
				  "INLINE-COMMENT",
				  2, (const char**)fltargs,
				  MU_FILTER_ENCODE, MU_STREAM_WRITE);
      mu_stream_unref (errstr);
      free (fltargs[1]);
      if (rc)
	{
	  fprintf (stderr,
		   "%s: cannot open output filter stream: %s",
		   mu_program_name ? mu_program_name : "<unknown>",
		   mu_strerror (rc));
	  abort ();
	}
      mu_stream_set_buffer (transport, mu_buffer_line, 0);
    }

  str->event_cb = NULL;
  str->event_mask = 0;
  str->event_cb_data = 0;

  _mu_log_stream_setup (logstr, transport);
  stdstream_flushall_setup ();
}

/* The noop destroy function is necessary to prevent stream core from
   freeing the stream on mu_stream_unref. */
static void
bootstrap_destroy (struct _mu_stream *str)
{
  /* Nothing */
}

/* Standard I/O streams: */
static struct _mu_file_stream stdstream[2] = {
  { { ref_count: 1,
      buftype: mu_buffer_none,
      flags: MU_STREAM_READ,
      destroy: bootstrap_destroy,
      event_cb: std_bootstrap,
      event_mask: _MU_STR_EVMASK (_MU_STR_EVENT_BOOTSTRAP)
    }, fd: MU_STDIN_FD, filename: "<stdin>",
    flags: _MU_FILE_STREAM_FD_BORROWED|_MU_FILE_STREAM_STATIC_FILENAME },
  { { ref_count: 1,
      buftype: mu_buffer_none,
      flags: MU_STREAM_WRITE,
      destroy: bootstrap_destroy,
      event_cb: std_bootstrap,
      event_mask: _MU_STR_EVMASK (_MU_STR_EVENT_BOOTSTRAP)
    }, fd: MU_STDOUT_FD, filename: "<stdout>",
    flags: _MU_FILE_STREAM_FD_BORROWED|_MU_FILE_STREAM_STATIC_FILENAME }
};

/* Standard error stream: */
static struct _mu_log_stream default_strerr = {
  { ref_count: 1,
    buftype: mu_buffer_none,
    flags: MU_STREAM_WRITE,
    destroy: bootstrap_destroy,
    event_cb: std_log_bootstrap,
    event_mask: _MU_STR_EVMASK (_MU_STR_EVENT_BOOTSTRAP)
  }
};

/* Pointers to these: */
mu_stream_t mu_strin  = (mu_stream_t) &stdstream[MU_STDIN_FD];
mu_stream_t mu_strout = (mu_stream_t) &stdstream[MU_STDOUT_FD];
mu_stream_t mu_strerr = (mu_stream_t) &default_strerr;

static void
stdstream_flushall (void *data MU_ARG_UNUSED)
{
  mu_stream_flush (mu_strin);
  mu_stream_flush (mu_strout);
  mu_stream_flush (mu_strerr);
}

static void
stdstream_flushall_setup (void)
{
  static int _setup = 0;

  if (!_setup)
    {
      mu_onexit (stdstream_flushall, NULL);
      _setup = 1;
    }
}

void
mu_stdstream_setup (int flags)
{
  int rc;
  int fd;
  int yes = 1;
  
  /* If the streams are already open, close them */
  if (flags & MU_STDSTREAM_RESET_STRIN)
    mu_stream_destroy (&mu_strin);
  if (flags & MU_STDSTREAM_RESET_STROUT)
    mu_stream_destroy (&mu_strout);
  if (flags & MU_STDSTREAM_RESET_STRERR)
    mu_stream_destroy (&mu_strerr);
  
  /* Ensure that first 3 descriptors are open in proper mode */
  fd = open ("/dev/null", O_RDWR);
  switch (fd)
    {
    case 0:
      /* Keep it and try to open 1 */
      fd = open ("/dev/null", O_WRONLY);
      if (fd != 1)
	{
	  if (fd > 2)
	    close (fd);
	  break;
	}

    case 1:
      /* keep it open and try 2 */
      fd = open ("/dev/null", O_WRONLY);
      if (fd != 2)
	close (fd);
      break;

    case 2:
      /* keep it open */;
      break;

    default:
      close (fd);
      break;
    }

  /* Create the corresponding streams */
  if (!mu_strin)
    {
      rc = mu_stdio_stream_create (&mu_strin, MU_STDIN_FD, 0);
      if (rc)
	{
	  fprintf (stderr, "mu_stdio_stream_create(%d): %s\n",
		   MU_STDIN_FD, mu_strerror (rc));
	  abort ();
	}
      mu_stream_ioctl (mu_strin, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
    }

  if (!mu_strout)
    {
      rc = mu_stdio_stream_create (&mu_strout, MU_STDOUT_FD, 0);
      if (rc)
	{
	  fprintf (stderr, "mu_stdio_stream_create(%d): %s\n",
		   MU_STDOUT_FD, mu_strerror (rc));
	  abort ();
	}
      mu_stream_ioctl (mu_strout, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);
    }

  if (!mu_strerr)
    {
      if (mu_stdstream_strerr_create (&mu_strerr, MU_STRERR_STDERR, 0, 0,
				      mu_program_name, NULL))
	abort ();
    }

  stdstream_flushall_setup ();
}

int
mu_printf (const char *fmt, ...)
{
  int rc;
  va_list ap;

  va_start (ap, fmt);
  rc = mu_stream_vprintf (mu_strout, fmt, ap);
  va_end (ap);
  return rc;
}
