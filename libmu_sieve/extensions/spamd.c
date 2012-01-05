/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GNU Mailutils.  If not, see
   <http://www.gnu.org/licenses/>. */

/* This module implements sieve extension test "spamd": an interface to
   the SpamAssassin spamd daemon. See "Usage:" below for the description */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <mailutils/sieve.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/filter.h>
#include <mailutils/stream.h>

#define DEFAULT_SPAMD_PORT 783


/* Auxiliary functions */

static int
spamd_connect_tcp (mu_sieve_machine_t mach, mu_stream_t *stream,
		   char *host, int port)
{
  int rc = mu_tcp_stream_create (stream, host, port, MU_STREAM_RDWR);
  if (rc)
    {
      mu_sieve_error (mach, "mu_tcp_stream_create: %s", mu_strerror (rc));
      return rc;
    }
  return rc;
}

static int
spamd_connect_socket (mu_sieve_machine_t mach, mu_stream_t *stream, char *path)
{
  int rc = mu_socket_stream_create (stream, path, MU_STREAM_RDWR);
  if (rc)
    {
      mu_sieve_error (mach, "mu_socket_stream_create: %s", mu_strerror (rc));
      return rc;
    }
  return rc;
}

static void
spamd_destroy (mu_stream_t *stream)
{
  mu_stream_close (*stream);
  mu_stream_destroy (stream);
}

static void
spamd_send_command (mu_stream_t stream, const char *fmt, ...)
{
  char buf[512];
  size_t n;
  va_list ap;

  va_start (ap, fmt);
  n = vsnprintf (buf, sizeof buf, fmt, ap);
  va_end (ap);
  mu_stream_writeline (stream, buf, n);
}

static int
spamd_send_message (mu_stream_t stream, mu_message_t msg, int dbg)
{
  int rc;
  mu_stream_t mstr, flt;
  struct mu_buffer_query newbuf, oldbuf;
  int bufchg = 0;
  mu_debug_handle_t dlev;
  int xlev;
  int xlevchg = 0;
  
  rc = mu_message_get_streamref (msg, &mstr);
  if (rc)
    return rc;
  rc = mu_filter_create (&flt, mstr, "CRLF", MU_FILTER_ENCODE,
			 MU_STREAM_READ|MU_STREAM_SEEK);
  if (rc)
    {
      mu_stream_destroy (&mstr);
      return rc;
    }

  /* Ensure effective transport buffering */
  if (mu_stream_ioctl (stream, MU_IOCTL_TRANSPORT_BUFFER,
		       MU_IOCTL_OP_GET, &oldbuf) == 0)
    {
      newbuf.type = MU_TRANSPORT_OUTPUT;
      newbuf.buftype = mu_buffer_full;
      newbuf.bufsize = 64*1024;
      mu_stream_ioctl (stream, MU_IOCTL_TRANSPORT_BUFFER, MU_IOCTL_OP_SET, 
		       &newbuf);
      bufchg = 1;
    }

  if (dbg &&
      mu_debug_category_level ("sieve", 5, &dlev) == 0 &&
      !(dlev & MU_DEBUG_LEVEL_MASK (MU_DEBUG_TRACE9)))
    {
      /* Mark out the following data as payload */
      xlev = MU_XSCRIPT_PAYLOAD;
      if (mu_stream_ioctl (stream, MU_IOCTL_XSCRIPTSTREAM,
			   MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev) == 0)
	xlevchg = 1;
    }
  
  rc = mu_stream_copy (stream, flt, 0, NULL);

  /* Restore prior transport buffering and xscript level */
  if (bufchg)
    mu_stream_ioctl (stream, MU_IOCTL_TRANSPORT_BUFFER, MU_IOCTL_OP_SET, 
		     &oldbuf);
  if (xlevchg)
    mu_stream_ioctl (stream, MU_IOCTL_XSCRIPTSTREAM,
		     MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev);
  
  mu_stream_destroy (&mstr);
  mu_stream_destroy (&flt);
  return rc;
}

#define char_to_num(c) (c-'0')

static void
decode_float (long *vn, const char *str, int digits, char **endp)
{
  long v;
  size_t frac = 0;
  size_t base = 1;
  int i;
  int negative = 0;
  char *end;
  
  for (i = 0; i < digits; i++)
    base *= 10;
  
  v = strtol (str, &end, 10);
  str = end;
  if (v < 0)
    {
      negative = 1;
      v = - v;
    }
  
  v *= base;
  if (*str == '.')
    {
      for (str++, i = 0; *str && mu_isdigit (*str) && i < digits;
	   i++, str++)
	frac = frac * 10 + char_to_num (*str);
      if (*str && mu_isdigit (*str))
	{
	  if (char_to_num (*str) >= 5)
	    frac++;
	  if (endp)
	    while (*str && mu_isdigit (*str))
	      str++;
	}
      else
	for (; i < digits; i++)
	  frac *= 10;
    }
  *vn = v + frac;
  if (negative)
    *vn = - *vn;
  if (endp)
    *endp = (char*) str;
}

static int
decode_boolean (char *str)
{
  if (mu_c_strcasecmp (str, "true") == 0)
    return 1;
  else if (mu_c_strcasecmp (str, "false") == 0)
    return 0;
  /*else?*/
  return 0;
}


/* Signal handling */

typedef RETSIGTYPE (*signal_handler)(int);

static signal_handler
set_signal_handler (int sig, signal_handler h)
{
#ifdef HAVE_SIGACTION
  struct sigaction act, oldact;
  act.sa_handler = h;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
  sigaction (sig, &act, &oldact);
  return oldact.sa_handler;
#else
  return signal (sig, h);
#endif
}

void
spamd_abort (mu_sieve_machine_t mach, mu_stream_t *stream, signal_handler handler)
{
  spamd_destroy (stream);
  set_signal_handler (SIGPIPE, handler);
  mu_sieve_abort (mach);
}

static int got_sigpipe;
static signal_handler handler;

static RETSIGTYPE
sigpipe_handler (int sig MU_ARG_UNUSED)
{
  got_sigpipe = 1;
}

static void
spamd_read_line (mu_sieve_machine_t mach, mu_stream_t stream,
		 char **pbuffer, size_t *psize)
{
  size_t n;
  int rc = mu_stream_getline (stream, pbuffer, psize, &n);
  if (rc == 0)
    mu_rtrim_class (*pbuffer, MU_CTYPE_ENDLN);
  else
    {
      /* FIXME: Need an 'onabort' mechanism in Sieve machine, which
	 would restore the things to their prior state.  This will
	 also allow to make handler local again. */
      free (pbuffer);
      mu_sieve_error (mach, "read error: %s", mu_strerror (rc));
      spamd_abort (mach, &stream, handler);
    }
}

static int
parse_response_line (mu_sieve_machine_t mach, const char *buffer)
{
  const char *str;
  char *end;
  long version;
  unsigned long resp;
  
  str = buffer;
  if (strncmp (str, "SPAMD/", 6))
    return MU_ERR_BADREPLY;
  str += 6;

  decode_float (&version, str, 1, &end);
  if (version < 10)
    {
      mu_sieve_error (mach, _("unsupported SPAMD version: %s"), str);
      return MU_ERR_BADREPLY;
    }

  str = mu_str_skip_class (end, MU_CTYPE_SPACE);
  if (!*str || !mu_isdigit (*str))
    {
      mu_sieve_error (mach, _("malformed spamd response: %s"), buffer);
      return MU_ERR_BADREPLY;
    }

  resp = strtoul (str, &end, 10);
  if (resp != 0)
    {
      mu_sieve_error (mach, _("spamd failure: %lu %s"), resp, end);
      return MU_ERR_REPLY;
    }
  return 0;
}

/* Compute the "real" size of the message.  This takes into account filtering
   applied by spamd_send_message (LF->CRLF transcription).

   FIXME: Previous versions used mu_message_size, but it turned out to be
   unreliable, because it sometimes returns a "normalized" size, which differs
   from the real one.  This happens when the underlying implementation does
   not provide a _get_size method, so that the size is computed as a sum of
   message body and header sizes.  This latter is returned by mu_header_size,
   which ignores extra whitespace around each semicolon (see header_parse in
   libmailutils/mailbox/header.c).
 */
static int
get_real_message_size (mu_message_t msg, size_t *psize)
{
  mu_stream_t null;
  mu_stream_stat_buffer stat;
  int rc;
  
  rc = mu_nullstream_create (&null, MU_STREAM_WRITE);
  if (rc)
    return rc;
  mu_stream_set_stat (null, MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT), stat);
  rc = spamd_send_message (null, msg, 0);
  mu_stream_destroy (&null);
  if (rc == 0)
    *psize = stat[MU_STREAM_STAT_OUT];
  return rc;
}

/* The test proper */

/* Syntax: spamd [":host" <tcp-host: string>]
                 [":port" <tcp-port: number> /
                  ":socket" <unix-socket: string>]
		 [":user" <name: string>] 
		 [":over" / ":under" <limit: string>]

   The "spamd" test is an interface to "spamd" facility of
   SpamAssassin mail filter. It evaluates to true if SpamAssassin
   recognized the message as spam, or the message spam score
   satisfies the given relation.

   If the argument is ":over" and the spam score is greater than
   or equal to the number provided, the test is true; otherwise,
   it is false.

   If the argument is ":under" and the spam score is less than
   or equal to the number provided, the test is true; otherwise,
   it is false.

   Spam score is a floating point number. The comparison takes into
   account three decimal digits.

*/

static int
spamd_test (mu_sieve_machine_t mach, mu_list_t args, mu_list_t tags)
{
  char *buffer = NULL;
  size_t size;
  char spam_str[6], score_str[21], threshold_str[21];
  int rc;
  int result;
  long score, threshold, limit;
  mu_stream_t stream = NULL, null;
  mu_sieve_value_t *arg;
  mu_message_t msg;
  char *host;
  mu_header_t hdr;
  mu_debug_handle_t lev = 0;
  
  if (mu_sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      mu_sieve_debug (mach, "spamd_test %lu",
		      (unsigned long) mu_sieve_get_message_num (mach));
    }
  
  if (mu_sieve_is_dry_run (mach))
    return 0;
  
  msg = mu_sieve_get_message (mach);
  rc = get_real_message_size (msg, &size);
  if (rc)
    {
      mu_sieve_error (mach, _("cannot get real message size: %s"),
		      mu_strerror (rc));
      mu_sieve_abort (mach);
    }
  
  if (mu_sieve_tag_lookup (tags, "host", &arg))
    host = arg->v.string;
  else
    host = "127.0.0.1";
  
  if (mu_sieve_tag_lookup (tags, "port", &arg))
    result = spamd_connect_tcp (mach, &stream, host, arg->v.number);
  else if (mu_sieve_tag_lookup (tags, "socket", &arg))
    result = spamd_connect_socket (mach, &stream, arg->v.string);
  else
    result = spamd_connect_tcp (mach, &stream, host, DEFAULT_SPAMD_PORT);
  if (result) /* spamd_connect_ already reported error */
    mu_sieve_abort (mach);

  mu_stream_set_buffer (stream, mu_buffer_line, 0);
  if (mu_debug_category_level ("sieve", 5, &lev) == 0 &&
      (lev & MU_DEBUG_LEVEL_MASK (MU_DEBUG_PROT)))
    {
      int rc;
      mu_stream_t dstr, xstr;
      
      rc = mu_dbgstream_create (&dstr, MU_DIAG_DEBUG);
      if (rc)
	mu_error (_("cannot create debug stream; transcript disabled: %s"),
		  mu_strerror (rc));
      else
	{
	  rc = mu_xscript_stream_create (&xstr, stream, dstr, NULL);
	  if (rc)
	    mu_error (_("cannot create transcript stream: %s"),
		      mu_strerror (rc));
	  else
	    {
	      mu_stream_unref (stream);
	      stream = xstr;
	    }
	}
    }

  spamd_send_command (stream, "SYMBOLS SPAMC/1.2");
  
  spamd_send_command (stream, "Content-length: %lu", (u_long) size);
  if (mu_sieve_tag_lookup (tags, "user", &arg))
    spamd_send_command (stream, "User: %s", arg);
  else
    {
      struct mu_auth_data *auth = mu_get_auth_by_uid (geteuid ());
      spamd_send_command (stream, "User: %s", auth ? auth->name : "root");
      mu_auth_data_free (auth);
    }

  got_sigpipe = 0;
  handler = set_signal_handler (SIGPIPE, sigpipe_handler);
  
  spamd_send_command (stream, "");
  spamd_send_message (stream, msg, 1);
  mu_stream_shutdown (stream, MU_STREAM_WRITE);

  spamd_read_line (mach, stream, &buffer, &size);

  if (got_sigpipe)
    {
      mu_sieve_error (mach, _("remote side has closed connection"));
      spamd_abort (mach, &stream, handler);
    }

  if (parse_response_line (mach, buffer))
    spamd_abort (mach, &stream, handler);
  
  spamd_read_line (mach, stream, &buffer, &size);
  if (sscanf (buffer, "Spam: %5s ; %20s / %20s",
	      spam_str, score_str, threshold_str) != 3)
    {
      mu_sieve_error (mach, _("spamd responded with bad Spam header '%s'"), 
                      buffer);
      spamd_abort (mach, &stream, handler);
    }

  result = decode_boolean (spam_str);
  score = strtoul (score_str, NULL, 10);
  decode_float (&score, score_str, 3, NULL);
  decode_float (&threshold, threshold_str, 3, NULL);

  if (!result)
    {
      if (mu_sieve_tag_lookup (tags, "over", &arg))
	{
	  decode_float (&limit, arg->v.string, 3, NULL);
	  result = score >= limit;
	}
      else if (mu_sieve_tag_lookup (tags, "under", &arg))
	{
	  decode_float (&limit, arg->v.string, 3, NULL);
	  result = score <= limit;	  
	}
    }
  
  /* Skip newline */
  spamd_read_line (mach, stream, &buffer, &size);
  /* Read symbol list */
  spamd_read_line (mach, stream, &buffer, &size);

  rc = mu_message_get_header (msg, &hdr);
  if (rc)
    {
      mu_sieve_error (mach, _("cannot get message header: %s"), 
                      mu_strerror (rc));
      spamd_abort (mach, &stream, handler);
    }

  mu_header_append (hdr, "X-Spamd-Status", spam_str);
  mu_header_append (hdr, "X-Spamd-Score", score_str);
  mu_header_append (hdr, "X-Spamd-Threshold", threshold_str);
  mu_header_append (hdr, "X-Spamd-Keywords", buffer);

  free (buffer);

  /* Create a data sink */
  mu_nullstream_create (&null, MU_STREAM_WRITE);

  /* Mark out the following data as payload */
  if (!(lev & MU_DEBUG_LEVEL_MASK (MU_DEBUG_TRACE9)))
    {
      int xlev = MU_XSCRIPT_PAYLOAD;
      mu_stream_ioctl (stream, MU_IOCTL_XSCRIPTSTREAM,
		       MU_IOCTL_XSCRIPTSTREAM_LEVEL, &xlev);
    }
  mu_stream_copy (null, stream, 0, NULL);
  mu_stream_destroy (&null);
  mu_stream_destroy (&stream);

  set_signal_handler (SIGPIPE, handler);

  return result;
}


/* Initialization */
   
/* Required arguments: */
static mu_sieve_data_type spamd_req_args[] = {
  SVT_VOID
};

/* Tagged arguments: */
static mu_sieve_tag_def_t spamd_tags[] = {
  { "host", SVT_STRING },
  { "port", SVT_NUMBER },
  { "socket", SVT_STRING },
  { "user", SVT_STRING },
  { "over", SVT_STRING },
  { "under", SVT_STRING },
  { NULL }
};

static mu_sieve_tag_group_t spamd_tag_groups[] = {
  { spamd_tags, NULL },
  { NULL }
};


/* Initialization function. */
int
SIEVE_EXPORT(spamd,init) (mu_sieve_machine_t mach)
{
  return mu_sieve_register_test (mach, "spamd", spamd_test,
                              spamd_req_args, spamd_tag_groups, 1);
}
   
