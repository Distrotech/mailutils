/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GNU Mailutils; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

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
#include <mailutils/libsieve.h>
#include <mailutils/mu_auth.h>

#define DEFAULT_SPAMD_PORT 783


/* Auxiliary functions */

static int
spamd_connect_tcp (mu_sieve_machine_t mach, stream_t *stream,
		   char *host, int port)
{
  int rc = tcp_stream_create (stream, host, port, 0);
  if (rc)
    {
      mu_sieve_error (mach, "tcp_stream_create: %s", mu_strerror (rc));
      return rc;
    }
  rc = stream_open (*stream);
  if (rc)
    mu_sieve_error (mach, "opening tcp stream: %s", mu_strerror (rc));
  return rc;
}

static int
spamd_connect_socket (mu_sieve_machine_t mach, stream_t *stream, char *path)
{
  /* FIXME: A library deficiency: we cannot create a unix socket stream */
  int fd, rc;
  FILE *fp;
  struct sockaddr_un addr;
  
  if ((fd = socket (PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      mu_sieve_error (mach, "socket: %s", mu_strerror (errno));
      return errno;
    }

  memset (&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  strncpy (addr.sun_path, path, sizeof addr.sun_path - 1);
  addr.sun_path[sizeof addr.sun_path - 1] = 0;
  if (connect (fd, (struct sockaddr *) &addr, sizeof(addr)))
    {
      mu_sieve_error (mach, "connect: %s", mu_strerror (errno));
      close (fd);
      return errno;
    }

  fp = fdopen (fd, "w+");
  rc = stdio_stream_create (stream, fp, MU_STREAM_RDWR);
  if (rc)
    {
      mu_sieve_error (mach, "stdio_stream_create: %s", mu_strerror (rc));
      fclose (fp);
      return rc;
    }

  rc = stream_open (*stream);
  if (rc)
    {
      mu_sieve_error (mach, "stream_open: %s", mu_strerror (rc));
      stream_destroy (stream, stream_get_owner (*stream));
    }
  return rc;
}

static void
spamd_destroy (stream_t *stream)
{
  stream_close (*stream);
  stream_destroy (stream, stream_get_owner (*stream));
}

static void
spamd_shutdown (stream_t stream, int flag)
{
  mu_transport_t trans;
  stream_flush (stream);
  stream_get_transport (stream, &trans);
  shutdown (fileno ((FILE*)trans), flag);
}

static void
spamd_send_command (stream_t stream, const char *fmt, ...)
{
  char buf[512];
  size_t n;
  va_list ap;

  va_start (ap, fmt);
  n = vsnprintf (buf, sizeof buf, fmt, ap);
  va_end (ap);
  stream_sequential_write (stream, buf, n);
  stream_sequential_write (stream, "\r\n", 2);
}

static void
spamd_send_message (stream_t stream, message_t msg)
{
  size_t size;
  char buf[512];
  stream_t mstr;

  message_get_stream (msg, &mstr);
  stream_seek (mstr, 0, SEEK_SET);
  while (stream_sequential_readline (mstr, buf, sizeof (buf), &size) == 0
	 && size > 0)
    {
      char *nl = NULL;
      
      if (buf[size-1] == '\n')
	{
	  size--;
	  nl = "\r\n";
	}
      stream_sequential_write (stream, buf, size);
      if (nl)
	stream_sequential_write (stream, nl, 2);
    }
}

static size_t
spamd_read_line (mu_sieve_machine_t mach, stream_t stream,
		 char *buffer, size_t size, size_t *pn)
{
  size_t n = 0;
  int rc = stream_sequential_readline (stream, buffer, size, &n);
  if (rc == 0)
    {
      if (pn)
	*pn = n;
      while (n > 0 && (buffer[n-1] == '\r' || buffer[n-1] == '\n'))
	n--;
      buffer[n] = 0;
      if (mu_sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
	mu_sieve_debug (mach, ">> %s\n", buffer);
    }
  return rc;
}

#define char_to_num(c) (c-'0')

static void
decode_float (long *vn, char *str, int digits)
{
  long v;
  size_t frac = 0;
  size_t base = 1;
  int i;
  int negative = 0;
  
  for (i = 0; i < digits; i++)
    base *= 10;
  
  v = strtol (str, &str, 10);
  if (v < 0)
    {
      negative = 1;
      v = - v;
    }
  
  v *= base;
  if (*str == '.')
    {
      for (str++, i = 0; *str && i < digits; i++, str++)
	frac = frac * 10 + char_to_num (*str);
      if (*str)
	{
	  if (char_to_num (*str) >= 5)
	    frac++;
	}
      else
	for (; i < digits; i++)
	  frac *= 10;
    }
  *vn = v + frac;
  if (negative)
    *vn = - *vn;
}

static int
decode_boolean (char *str)
{
  if (strcasecmp (str, "true") == 0)
    return 1;
  else if (strcasecmp (str, "false") == 0)
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
spamd_abort (mu_sieve_machine_t mach, stream_t *stream, signal_handler handler)
{
  spamd_destroy (stream);
  set_signal_handler (SIGPIPE, handler);
  mu_sieve_abort (mach);
}

static int got_sigpipe;

static RETSIGTYPE
sigpipe_handler (int sig ARG_UNUSED)
{
  got_sigpipe = 1;
}


/* The test proper */

/* Syntax: spamd [":host" <tcp-host: string]
                 [":port" <tcp-port: number> /
                  ":socket" <unix-socket: string>]
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
spamd_test (mu_sieve_machine_t mach, list_t args, list_t tags)
{
  char buffer[512];
  char version_str[19];
  char spam_str[6], score_str[21], threshold_str[21];
  int response, rc;
  long version;
  int result;
  long score, threshold, limit;
  stream_t stream = NULL;
  mu_sieve_value_t *arg;
  message_t msg;
  size_t m_size, m_lines, size;
  struct mu_auth_data *auth;
  signal_handler handler;
  char *host;
  header_t hdr;

  if (mu_sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    {
      mu_sieve_locus_t locus;
      mu_sieve_get_locus (mach, &locus);
      mu_sieve_debug (mach, "%s:%lu: spamd_test %lu\n",
		   locus.source_file,
		   (unsigned long) locus.source_line,
		   (u_long) mu_sieve_get_message_num (mach));
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

  msg = mu_sieve_get_message (mach);
  message_size (msg, &m_size);
  message_lines (msg, &m_lines);

  auth = mu_get_auth_by_uid (geteuid ());
  spamd_send_command (stream, "SYMBOLS SPAMC/1.2");
  spamd_send_command (stream, "Content-length: %lu",
		      (u_long) (m_size + m_lines));
  spamd_send_command (stream, "User: %s", auth ? auth->name : "root");
  mu_auth_data_free (auth);

  got_sigpipe = 0;
  handler = set_signal_handler (SIGPIPE, sigpipe_handler);
  
  spamd_send_command (stream, "");
  spamd_send_message (stream, msg);
  spamd_shutdown (stream, SHUT_WR);

  spamd_read_line (mach, stream, buffer, sizeof buffer, NULL);

  if (got_sigpipe)
    {
      mu_sieve_error (mach, "remote side has closed connection");
      spamd_abort (mach, &stream, handler);
    }

  if (sscanf (buffer, "SPAMD/%18s %d %*s", version_str, &response) != 2)
    {
      mu_sieve_error (mach, "spamd responded with bad string '%s'", buffer);
      spamd_abort (mach, &stream, handler);
    }
  
  decode_float (&version, version_str, 1);
  if (version < 10)
    {
      mu_sieve_error (mach, "unsupported SPAMD version: %s", version_str);
      spamd_abort (mach, &stream, handler);
    }

  /*
  if (response)
    ...
  */
  
  spamd_read_line (mach, stream, buffer, sizeof buffer, NULL);
  if (sscanf (buffer, "Spam: %5s ; %20s / %20s",
	      spam_str, score_str, threshold_str) != 3)
    {
      mu_sieve_error (mach, "spamd responded with bad Spam header '%s'", buffer);
      spamd_abort (mach, &stream, handler);
    }

  result = decode_boolean (spam_str);
  score = strtoul (score_str, NULL, 10);
  decode_float (&score, score_str, 3);
  decode_float (&threshold, threshold_str, 3);

  if (!result)
    {
      if (mu_sieve_tag_lookup (tags, "over", &arg))
	{
	  decode_float (&limit, arg->v.string, 3);
	  result = score >= limit;
	}
      else if (mu_sieve_tag_lookup (tags, "over", &arg))
	{
	  decode_float (&limit, arg->v.string, 3);
	  result = score <= limit;	  
	}
    }
  
  /* Skip newline */
  spamd_read_line (mach, stream, buffer, sizeof buffer, NULL);
  /* Read symbol list */
  spamd_read_line (mach, stream, buffer, sizeof buffer, &size);

  rc = message_get_header (msg, &hdr);
  if (rc)
    {
      mu_sieve_error (mach, "cannot get message header: %s", mu_strerror (rc));
      spamd_abort (mach, &stream, handler);
    }

  mu_header_set_value (hdr, "X-Spamd-Status", spam_str, 1);
  mu_header_set_value (hdr, "X-Spamd-Score", score_str, 1);
  mu_header_set_value (hdr, "X-Spamd-Threshold", threshold_str, 1);
  mu_header_set_value (hdr, "X-Spamd-Keywords", buffer, 1);

  while (spamd_read_line (mach, stream, buffer, sizeof buffer, &size) == 0
	 && size > 0)
    /* Drain input */;
  
  spamd_destroy (&stream);
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
   
