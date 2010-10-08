/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2007, 2009, 2010 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

/* This is a "fake" mta designed for testing purposes. It imitates
   sendmail sending and daemon modes. It does not actually send anything,
   instead it just outputs a transcript of what would have been done.

   Invocation:
   
   1. mta [-t][-oi][-f from-person] [recipients]

   Reads the message from the standard input and delivers it to each of the
   recipients.

   2. mta -bd [-p port]

   Operates as daemon. If port is given, mta will listen on that port.
   Otherwise, it will use the first free port in the range 1024-65535.
   In this case, mta prints the port number on the stdout, prior to
   starting operation. Notice, that in this mode mta does not disconnect
   itself from the controlling terminal, it always stays on the foreground.

   Environment variables:

   MTA_DOMAIN   Sets the default domain name for email addresses. Default
                is "localhost".
   MTA_DIAG     Sets the name of the output diagnostic file. By default,
                the diagnostics goes to stderr.
   MTA_APPEND   When set to any non-empty value, directs mta to append
                to the diagnostics file, not to overwrite it. 

*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
#include <string.h>
#include <pwd.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mailutils/argcv.h>
#include <mailutils/mailutils.h>

FILE *diag = NULL;       /* diagnostic output */
int smtp_mode;           /* Operate in smtp mode */
int port = 0;            /* Port number (for smtp mode) */
char *from_person = NULL; /* Set the name of the `from' person */
int read_recipients = 0; /* Read the message for recipients */
int dot = 1;             /* Message is terminated by a lone dot on a line */

mu_address_t recipients = NULL;
char *progname;
/* FIXME: If finalize_option is set, mta should try to finalize
   received messages the way sendmail does, i.e. to add To: or
   Cc: headers, if they are missing, etc. The code to do so is
   already included, but the modified message is not reproduced
   on diagnostic output because mta_send reads it from the stream,
   which does not reflect modifications to the header.

   Ideally, the mu_message_get_streamref function should notice the
   change in the header (and/or the body) and return a temporary
   stream, which will read the modified values.  This is left as
   as TODO for a later time.                  2010-09-29, Sergey */
int finalize_option = 0; 

int mta_stdin (int argc, char **argv);
int mta_smtp (int argc, char **argv);
void register_handlers (void);

int
main (int argc, char **argv)
{
  int c, status;
  char *domain;

  progname = strrchr (argv[0], '/');
  if (!progname)
    progname = argv[0];
  
  while ((c = getopt (argc, argv, "b:f:p:to:")) != EOF)
    {
      switch (c)
	{
	case 'b':
	  switch (optarg[0])
	    {
	    case 'd':
	      smtp_mode = 1;
	      break;

	    default:
	      /*FIXME*/;
	    }
	  break;
	  
	case 'f':
	  from_person = optarg;
	  break;
	  
	case 'p':
	  port = strtoul (optarg, NULL, 0);
	  break;
	  
	case 't':
	  read_recipients = 1;
	  break;

	case 'o':
	  switch (optarg[0])
	    {
	    case 'i':
	      dot = 0;
	      break;

	    default:
	      /* FIXME */ ;
	    }
	  break;

	default:
	  exit (EX_USAGE);
	}
    }

  if (!diag)
    {
      char *name = getenv ("MTA_DIAG");
      if (name)
	{
	  char *mode = getenv ("MTA_APPEND") ? "a" : "w";
	  diag = fopen (name, mode);
	  if (!diag)
	    {
	      mu_error ("%s: can't open diagnostic output: %s",
			progname, name);
	      return 1;
	    }
	}
      else
	diag = stdout;
    }
  
  register_handlers ();

  domain = getenv ("MTA_DOMAIN");
  mu_set_user_email_domain (domain ? domain : "localhost");
  
  argc -= optind;
  argv += optind;
  if (smtp_mode)
    status = mta_smtp (argc, argv);
  else
    status = mta_stdin (argc, argv);
  fclose (diag);
  return status;
}

char *
from_address ()
{
  if (!from_person)
    {
      struct passwd *pwd = getpwuid (getuid ());
      if (pwd)
	return mu_get_user_email (pwd->pw_name);
    }
  return strdup (from_person);
}
     
static mu_message_t
make_tmp (mu_stream_t in)
{
  int rc;
  mu_stream_t out;
  char *buf = NULL;
  size_t size = 0, n;
  mu_message_t mesg;
  
  rc = mu_temp_file_stream_create (&out, NULL);
  if (rc)
    {
      mu_error (_("unable to open temporary file: %s"), mu_strerror (rc));
      exit (EX_UNAVAILABLE);
    }

  rc = mu_stream_getline (in, &buf, &size, &n);
  if (rc)
    {
      mu_error (_("read error: %s"), mu_strerror (rc));
      mu_stream_destroy (&out);
      exit (EX_UNAVAILABLE);
    }
  if (n == 0)
    {
      mu_error (_("unexpected EOF on input"));
      mu_stream_destroy (&out);
      exit (EX_DATAERR);
    } 

  if (n >= 5 && memcmp (buf, "From ", 5))
    {
      time_t t;
      const char *from = from_address ();
      if (!from)
	{
	  mu_error ("%s: can't determine sender address", progname);
	  exit (EX_NOUSER);
	}
	  
      time (&t);
      mu_stream_printf (out, "From %s %s", from, ctime (&t));
    }

  mu_stream_write (out, buf, n, NULL);
  free (buf);
  
  rc = mu_stream_copy (out, in, 0, NULL);
  if (rc)
    {
      mu_error (_("copy error: %s"), mu_strerror (rc));
      mu_stream_destroy (&out);
      exit (EX_UNAVAILABLE);
    }

  rc = mu_stream_to_message (out, &mesg);
  mu_stream_destroy (&out);
  if (rc)
    {
      mu_error (_("error creating temporary message: %s"),
		mu_strerror (rc));
      exit (EX_UNAVAILABLE);
    }

  return mesg;
}

void
register_handlers ()
{
  mu_registrar_record (mu_mbox_record);
  mu_registrar_record (mu_sendmail_record);
  mu_registrar_record (mu_smtp_record);
  mu_registrar_set_default_record (mu_mbox_record);
}

int
add_recipient (const char *name)
{
  mu_address_t addr;
  int status;
    
  status = mu_address_create (&addr, name);
  if (status == 0)
    status = mu_address_union (&recipients, addr);
  mu_address_destroy (&addr);
  return status;
}

/* Convert addr to a comma-separated list of email addresses,
   suitable as an argument to RCPT TO command */
char *
address_email_string (mu_address_t addr)
{
  size_t count, i, n, length;
  char *value, *p;
  
  mu_address_get_email_count (addr, &count);
  length = 0;
  for (i = 1; i <= count; i++)
    {
      const char *str;
      mu_address_sget_email (recipients, i, &str);
      length += strlen (str) + 3;
    }

  value = malloc (length + 1);
  if (!value)
    {
      mu_error ("%s: not enough memory", progname);
      return NULL;
    }
  p = value;
  for (i = 1; i <= count; i++)
    {
      *p++ = '<';
      mu_address_get_email (recipients, i, p, length - (p - value), &n);
      p += n;
      *p++ = '>';
      if (i + 1 <= count)
	*p++ = ',';
    }
  *p = 0;
  return value;
}

int
mta_send (mu_message_t msg)
{
  size_t n;
  char buffer[512];    
  mu_stream_t stream = NULL;
  size_t line;
  char *value;
  int newline = 0;
  
  value = from_address ();
  if (value)
    {
      fprintf (diag, "ENVELOPE FROM: %s\n", value);
      free (value);
    }
  
  value = address_email_string (recipients);
  fprintf (diag, "ENVELOPE TO: %s\n", value);
  free (value);

  mu_message_get_streamref (msg, &stream);
  line = 0;
  newline = 1;
  while (mu_stream_read (stream, buffer, sizeof buffer - 1, &n) == 0
	 && n != 0)
    {
      size_t i;

      for (i = 0; i < n; i++)
	{
	  if (newline)
	    {
	      fprintf (diag, "%4lu: ", (unsigned long) line);
	      newline = 0;
	    }
	  fputc (buffer[i], diag);
	  if (buffer[i] == '\n')
	    {
	      line++;
	      newline = 1;
	    }
	}
    }
  mu_stream_destroy (&stream);
  if (!newline)
    fprintf (diag, "\n(no newline at EOF)\n");
  fprintf (diag, "END OF MESSAGE\n");
  fflush (diag);
  return 0;
}

#define SENDER_WARNING "set sender using -f flag"

int
message_finalize (mu_message_t msg, int warn)
{
  mu_header_t header = NULL;
  int have_to;
  char *value = NULL;
  
  mu_message_get_header (msg, &header);

  if (finalize_option && warn && from_person)
    {
      struct passwd *pwd = getpwuid (getuid ());
      char *warn = malloc (strlen (pwd->pw_name) + 1 +
			   sizeof (SENDER_WARNING));
      if (warn == NULL)
	{
	  mu_error ("%s: not enough memory", progname);
	  return 1;
	}
      sprintf (warn, "%s %s", pwd->pw_name, SENDER_WARNING);
      mu_header_set_value (header, "X-Authentication-Warning", warn, 0);
      free (warn);
    }
  
  have_to = mu_header_aget_value (header, MU_HEADER_TO, &value) == 0;
  
  if (read_recipients)
    {
      if (value)
	{
	  if (add_recipient (value))
	    {
	      mu_error ("%s: bad address %s", progname, value);
	      return 1;
	    }
	  free (value);
	}
	  
      if (mu_header_aget_value (header, MU_HEADER_CC, &value) == 0)
	{
	  if (add_recipient (value))
	    {
	      mu_error ("%s: bad address %s", progname, value);
	      return 1;
	    }
	  free (value);
	}  
	  
      if (finalize_option &&
	  mu_header_aget_value (header, MU_HEADER_BCC, &value) == 0)
	{
	  if (add_recipient (value))
	    {
	      mu_error ("%s: bad address %s", progname, value);
	      return 1;
	    }
	  free (value);
	  mu_header_set_value (header, MU_HEADER_BCC, NULL, 1);
	}  
    }

  if (finalize_option && !have_to)
    {
      size_t n;
      int c;
      
      c = mu_address_to_string (recipients, NULL, 0, &n);
      if (c)
	{
	  mu_error ("%s: mu_address_to_string failure: %s",
		    progname, mu_strerror (c));
	  return 1;
	}
      value = malloc (n + 1);
      if (!value)
	{
	  mu_error ("%s: not enough memory", progname);
	  return 1;
	}

      mu_address_to_string (recipients, value, n + 1, &n);
      mu_header_set_value (header, MU_HEADER_TO, value, 1);
      free (value);
    }
  return 0;
}

int
mta_stdin (int argc, char **argv)
{
  int c, rc;
  mu_stream_t in;
  mu_message_t msg = NULL;
  
  for (c = 0; c < argc; c++)
    {
      if (add_recipient (argv[c]))
	{
	  mu_error ("%s: bad address %s", progname, argv[c]);
	  return 1;
	}
    }

  rc = mu_stdio_stream_create (&in, MU_STDIN_FD, MU_STREAM_READ);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stdio_stream_create",
		       "MU_STDIN_FD", rc);
      exit (EX_UNAVAILABLE);
    } 

  msg = make_tmp (in);
  mu_stream_destroy (&in);
  if (message_finalize (msg, 1))
    return 1;
  
  if (!recipients)
    {
      mu_error ("%s: Recipient names must be specified", progname);
      return 1;
    }

  mta_send (msg);
  mu_message_destroy (&msg, mu_message_get_owner (msg));
  return 0;
}

void
smtp_reply (mu_stream_t str, int code, char *fmt, ...)
{
  va_list ap;

  mu_stream_printf (str, "%d ", code);
  va_start (ap, fmt);
  mu_stream_vprintf (str, fmt, ap);
  va_end (ap);
  mu_stream_printf (str, "\r\n");
}

#define STATE_INIT   0
#define STATE_EHLO   1
#define STATE_MAIL   2
#define STATE_RCPT   3
#define STATE_DATA   4  
#define STATE_QUIT   5 
#define STATE_DOT    6

#define KW_EHLO      0
#define KW_HELO      1
#define KW_MAIL      2
#define KW_RCPT      3
#define KW_DATA      4   
#define KW_HELP      5
#define KW_QUIT      6

int
smtp_kw (const char *name)
{
  static struct kw {
    char *name;
    int code;
  } kw[] = {
    { "ehlo", KW_EHLO },      
    { "helo", KW_HELO },       
    { "mail", KW_MAIL },
    { "rcpt", KW_RCPT },
    { "data", KW_DATA },
    { "help", KW_HELP },
    { "quit", KW_QUIT },
    { NULL },
  };
  int i;

  for (i = 0; kw[i].name != NULL; i++)
    if (mu_c_strcasecmp (name, kw[i].name) == 0)
      return kw[i].code;
  return -1;
}

static char *
check_prefix (char *str, const char *prefix)
{
  int pflen = strlen (prefix);
  if (strlen (str) > pflen && mu_c_strncasecmp (str, prefix, pflen) == 0)
    return str + pflen;
  else
    return NULL;
}

void
smtp (mu_stream_t str)
{
  int state;
  char *buf = NULL;
  size_t size = 0;
  char *rcpt_addr;
  
  smtp_reply (str, 220, "Ready");
  for (state = STATE_INIT; state != STATE_QUIT; )
    {
      char *s;
      int argc;
      char **argv;
      int kw;
      size_t len;
      
      if (mu_stream_getline (str, &buf, &size, &len) || len == 0)
	exit (EX_PROTOCOL);

      s = mu_str_stripws (buf);
      
      if (mu_argcv_get (s, "", NULL, &argc, &argv))
	exit (EX_UNAVAILABLE);

      kw = smtp_kw (argv[0]);
      if (kw == KW_QUIT)
	{
	  smtp_reply (str, 221, "Done");
	  state = STATE_QUIT;
	  mu_argcv_free (argc, argv);
	  continue;
	}
      
      switch (state)
	{
	case STATE_INIT:
	  switch (kw)
	    {
	    case KW_EHLO:
	    case KW_HELO:
	      if (argc == 2)
		{
		  smtp_reply (str, 250, "pleased to meet you");
		  state = STATE_EHLO;
		}
	      else
		smtp_reply (str, 501, "%s requires domain address", argv[0]);
	      break;

	    default:
	      smtp_reply (str, 503, "Polite people say HELO first");
	      break;
	    }
	  break;
	  
	case STATE_EHLO:
	  switch (kw)
	    {
	    case KW_MAIL:
	      if (argc == 2)
		from_person = check_prefix (argv[1], "from:");
	      else if (argc == 3 && mu_c_strcasecmp (argv[1], "from:") == 0)
		from_person = argv[2];
	      else
		from_person = NULL;

	      if (from_person)
		{
		  from_person = strdup (from_person);
		  smtp_reply (str, 250, "Sender OK");
		  state = STATE_MAIL;
		}
	      else
		smtp_reply (str, 501, "Syntax error");
	      break;

	    default:
	      smtp_reply (str, 503, "Need MAIL command");
	    }
	  break;
	  
	case STATE_MAIL:
	  switch (kw)
	    {
	    case KW_RCPT:
	      if (argc == 2)
		rcpt_addr = check_prefix (argv[1], "to:");
	      else if (argc == 3 && mu_c_strcasecmp (argv[1], "to:") == 0)
		rcpt_addr = argv[2];
	      else
		rcpt_addr = NULL;
	      
	      if (rcpt_addr)
		{
		  if (add_recipient (rcpt_addr))
		    smtp_reply (str, 451, "Recipient not accepted");
		  else
		    {
		      smtp_reply (str, 250, "Recipient OK");
		      state = STATE_RCPT;
		    }
		}
	      else
		smtp_reply (str, 501, "Syntax error");
	      break;
	      
	    default:
	      smtp_reply (str, 503, "Need RCPT command");
	    }
	  break;
	  
	case STATE_RCPT:
	  switch (kw)
	    {
	    case KW_RCPT:
	      if (argc == 2)
		rcpt_addr = check_prefix (argv[1], "to:");
	      else if (argc == 3 && mu_c_strcasecmp (argv[1], "to:") == 0)
		rcpt_addr = argv[2];
	      else
		rcpt_addr = NULL;
	      
	      if (rcpt_addr)
		{
		  if (add_recipient (rcpt_addr))
		    smtp_reply (str, 451, "Recipient not accepted");
		  else
		    {
		      smtp_reply (str, 250, "Recipient OK");
		      state = STATE_RCPT;
		    }
		}
	      else
		smtp_reply (str, 501, "Syntax error");
	      break;

	    case KW_DATA:
	      {
		mu_stream_t flt;
		mu_message_t msg;
		int rc;
		
		rc = mu_filter_create (&flt, str, "CRLFDOT", MU_FILTER_DECODE,
				       MU_STREAM_READ|MU_STREAM_WRTHRU);
		if (rc)
		  {
		    mu_diag_funcall (MU_DIAG_ERROR, "mu_filter_create",
				     "CRLFDOT", rc);
		    exit (EX_UNAVAILABLE);
		  }
		smtp_reply (str, 354,
			    "Enter mail, end with \".\" on a line by itself");
		
		msg = make_tmp (flt);
		mu_stream_destroy (&flt);
		if (message_finalize (msg, 0) == 0)
		  mta_send (msg);
		else
		  smtp_reply (str, 501, "can't send message"); /*FIXME: code?*/
		mu_message_destroy (&msg, mu_message_get_owner (msg));
		mu_address_destroy (&recipients);
		from_person = NULL;
	      
		smtp_reply (str, 250, "Message accepted for delivery");
		state = STATE_EHLO;
	      }
	      break;

	    default:
	      smtp_reply (str, 503, "Invalid command");
	      break;
	    }
	  break;

	}
      mu_argcv_free (argc, argv);
    }
}

int
mta_smtp (int argc, char **argv)
{
  int on = 1;
  struct sockaddr_in address;
  int fd;
  
  fd = socket (PF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    {
      mu_error ("%s: socket: %s", progname, strerror (errno));
      return 1;
    }

  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

  memset (&address, 0, sizeof (address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;

  if (port)
    {
      address.sin_port = htons (port);
      if (bind (fd, (struct sockaddr *) &address, sizeof (address)) < 0)
	{
	  close (fd);
	  mu_error ("%s: bind: %s", progname, strerror(errno));
	  return 1;
	}
    }
  else
    {
      int status;
      
      port = 1023;
      do
	{
	  if (++port >= 65535)
	    {
	      mu_error ("%s: can't bind socket: all ports in use?",
			progname);
	      return 1;
	    }
	  address.sin_port = htons (port);
	  status = bind (fd, (struct sockaddr *) &address, sizeof (address));
	}
      while (status < 0);
    }

  listen (fd, 5);
  printf ("%d\n", port);
  fclose (stdout);
  while (1)
    {
      fd_set rfds;
      struct sockaddr_in his_addr;
      int sfd, status;
      socklen_t len;
      int rc;
      mu_stream_t str;
      
      FD_ZERO (&rfds);
      FD_SET (fd, &rfds);
      
      status = select (fd + 1, &rfds, NULL, NULL, NULL);
      if (status == -1)
	{
	  if (errno == EINTR)
	    continue;
	  mu_error ("%s: select: %s", progname, strerror (errno));
	  return 1;
	}

      len = sizeof (his_addr);
      if ((sfd = accept (fd, (struct sockaddr *)&his_addr, &len)) < 0)
	{
	  mu_error ("%s: accept: %s", progname, strerror (errno));
	  return 1;
	}

      rc = mu_fd_stream_create (&str, NULL, sfd,
				MU_STREAM_RDWR|MU_STREAM_AUTOCLOSE);
      if (rc)
	{
	  mu_diag_funcall (MU_DIAG_ERROR, "mu_fd_stream_create", NULL, rc);
	  break;
	}
      mu_stream_set_buffer (str, mu_buffer_line, 0);
      smtp (str);
      mu_stream_destroy (&str);
      break;
    }
  
  return 0;
}

