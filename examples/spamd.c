/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

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
spamd_connect_tcp (sieve_machine_t mach, stream_t *stream,
		   char *host, int port)
{
  int rc = tcp_stream_create (stream, host, port, 0);
  if (rc)
    {
      sieve_error (mach, "tcp_stream_create: %s", mu_strerror (rc));
      return rc;
    }
  rc = stream_open (*stream);
  if (rc)
    sieve_error (mach, "opening tcp stream: %s", mu_strerror (rc));
  return rc;
}

static int
spamd_connect_socket (sieve_machine_t mach, stream_t *stream, char *path)
{
  /* FIXME: A library deficiency: we cannot create a unix socket stream */
  int fd, rc;
  FILE *fp;
  struct sockaddr_un addr;
  
  if ((fd = socket (PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
      sieve_error (mach, "socket: %s", mu_strerror (errno));
      return errno;
    }

  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof addr.sun_path - 1);
  addr.sun_path[sizeof addr.sun_path - 1] = 0;
  if (connect (fd, (struct sockaddr *) &addr, sizeof(addr)))
    {
      sieve_error (mach, "connect: %s", mu_strerror (errno));
      close (fd);
      return errno;
    }

  fp = fdopen (fd, "w+");
  rc = stdio_stream_create (stream, fp, MU_STREAM_RDWR);
  if (rc)
    {
      sieve_error (mach, "stdio_stream_create: %s", mu_strerror (rc));
      fclose (fp);
      return rc;
    }

  rc = stream_open (*stream);
  if (rc)
    {
      sieve_error (mach, "stream_open: %s", mu_strerror (rc));
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
  int fd;
  stream_flush (stream);
  stream_get_fd (stream, &fd);
  shutdown (fd, flag);
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
spamd_read_line (sieve_machine_t mach, stream_t stream,
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
      if (sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
	sieve_debug (mach, ">> %s\n", buffer);
    }
  return rc;
}

static void
decode_float (size_t *vn, char *str, int base)
{
  *vn = strtoul (str, &str, 10);
  *vn *= base;
  if (*str == '.')
    *vn += strtoul (str+1, NULL, 10);
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

static int
waitdebug()
{
  static volatile int _st=0;
  while (!_st)
    _st=_st;
}

struct symbol_closure {
  char *symbol;
  int found;
};

static int
_symbol_compare (void *item, void *data)
{
  char *symbol = item;
  struct symbol_closure *sp = data;
  return strcmp (symbol, sp->symbol) == 0;
}    

int
dummy_retrieve (void *item, void *data, int idx, char **pval)
{
  char **pbuf = data;
  char *p = strtok (*pbuf, ",");
  *pbuf = NULL;
  if (p)
    {
      *pval = strdup (p);
      return 0;
    }
  
  return 1;
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
spamd_abort (sieve_machine_t mach, stream_t *stream, signal_handler handler)
{
  spamd_destroy (stream);
  set_signal_handler (SIGPIPE, handler);
  sieve_abort (mach);
}

static int got_sigpipe;

static RETSIGTYPE
sigpipe_handler (int sig ARG_UNUSED)
{
  printf("PIPE!\n");
  got_sigpipe = 1;
}


/* The test proper */

/* Syntax: spamd [":host" <tcp-host: string]
                 [":port" <tcp-port: number> /
                  ":socket" <unix-socket: string>]
		 [":over" / ":under" <limit: number>]
		 [COMPARATOR] [MATCH-TYPE]
		 <symbols: string-list>

*/

static int
spamd_test (sieve_machine_t mach, list_t args, list_t tags)
{
  char buffer[512];
  char version_str[19];
  char spam_str[6], score_str[21], threshold_str[21];
  int response;
  size_t version;
  int result;
  size_t score, threshold, limit;
  stream_t stream = NULL;
  sieve_value_t *arg;
  message_t msg;
  size_t m_size, m_lines, size;
  struct mu_auth_data *auth;
  sieve_comparator_t comp = sieve_get_comparator (mach, tags);
  signal_handler handler;
  char *host;
  
  if (sieve_get_debug_level (mach) & MU_SIEVE_DEBUG_TRACE)
    sieve_debug (mach, "spamd_test %lu\n",
		 (u_long) sieve_get_message_num (mach));
  
  if (sieve_tag_lookup (tags, "host", &arg))
    host = arg->v.string;
  else
    host = "127.0.0.1";
  
  if (sieve_tag_lookup (tags, "port", &arg))
    result = spamd_connect_tcp (mach, &stream, host, arg->v.number);
  else if (sieve_tag_lookup (tags, "socket", &arg))
    result = spamd_connect_socket (mach, &stream, arg->v.string);
  else
    result = spamd_connect_tcp (mach, &stream, host, DEFAULT_SPAMD_PORT);
  if (result) /* spamd_connect_ already reported error */
    sieve_abort (mach);

  msg = sieve_get_message (mach);
  message_size (msg, &m_size);
  message_lines (msg, &m_lines);

  auth = mu_get_auth_by_uid (getuid ());
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
      sieve_error (mach, "remote side has closed connection");
      spamd_abort (mach, &stream, handler);
    }

  if (sscanf (buffer, "SPAMD/%18s %d %*s", version_str, &response) != 2)
    {
      sieve_error (mach, "spamd responded with bad string '%s'", buffer);
      spamd_abort (mach, &stream, handler);
    }
  
  decode_float (&version, version_str, 10);
  if (version < 10)
    {
      sieve_error (mach, "unsupported SPAMD version: %s", version_str);
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
      sieve_error (mach, "spamd responded with bad Spam header '%s'", buffer);
      spamd_abort (mach, &stream, handler);
    }

  result = decode_boolean (spam_str);
  score = strtoul (score_str, NULL, 10);
  decode_float (&score, score_str, 100);
  decode_float (&threshold, threshold_str, 100);

  if (!result)
    {
      if (sieve_tag_lookup (tags, "over", &arg))
	{
	  decode_float (&limit, arg->v.string, 100);
	  result = score >= limit;
	}
      else if (sieve_tag_lookup (tags, "over", &arg))
	{
	  decode_float (&limit, arg->v.string, 100);
	  result = score <= limit;	  
	}
    }
  
  if (!result)
    {
      /* Skip newline */
      spamd_read_line (mach, stream, buffer, sizeof buffer, NULL);
      /* Read symbol list */
      spamd_read_line (mach, stream, buffer, sizeof buffer, &size);
      if (buffer[0])
	{
	  list_t list;
	  sieve_value_t v;
	  char *p;
	  
	  list_create (&list);
	  list_append (list, buffer);
	  v.type = SVT_STRING_LIST;
	  v.v.list = list;
	  p = &buffer;
	  arg = sieve_value_get (args, 0);
	  result = sieve_vlist_compare (&v, arg,
					comp, sieve_get_relcmp (mach, tags),
					dummy_retrieve, &p, NULL) > 0;
	  list_destroy (&list);
	}
    }
  
  while (spamd_read_line (mach, stream, buffer, sizeof buffer, &size) == 0
	 && size > 0)
    /* Drain input */;

  spamd_destroy (&stream);
  set_signal_handler (SIGPIPE, handler);
  
  return result;
}


/* Initialization */
   
/* Required arguments: */
static sieve_data_type spamd_req_args[] = {
  SVT_STRING_LIST,
  SVT_VOID
};

/* Tagged arguments: */
static sieve_tag_def_t spamd_tags[] = {
  { "host", SVT_STRING },
  { "port", SVT_NUMBER },
  { "socket", SVT_STRING },
  { "over", SVT_STRING },
  { "under", SVT_STRING },
  { NULL }
};

static sieve_tag_def_t match_part_tags[] = {
  { "is", SVT_VOID },
  { "contains", SVT_VOID },
  { "matches", SVT_VOID },
  { "regex", SVT_VOID },
  { "count", SVT_STRING },
  { "value", SVT_STRING },
  { "comparator", SVT_STRING },
  { NULL }
};

static sieve_tag_group_t spamd_tag_groups[] = {
  { spamd_tags, NULL },
  { match_part_tags, sieve_match_part_checker },
  { NULL }
};


/* Initialization function. */
int
SIEVE_EXPORT(spamd,init) (sieve_machine_t mach)
{
  return sieve_register_test (mach, "spamd", spamd_test,
                              spamd_req_args, spamd_tag_groups, 1);
}
   
