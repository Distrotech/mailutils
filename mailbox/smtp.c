/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include <mailutils/stream.h>
#include <mailer0.h>
#include <registrar0.h>
#include <bio.h>
#include <misc.h>

static int smtp_init (mailer_t);

static struct mailer_entry _smtp_entry =
{
  url_smtp_init, smtp_init
};
static struct _record _smtp_record =
{
  MU_SMTP_SCHEME,
  NULL, /* Mailbox entry.  */
  &_smtp_entry, /* Mailer entry.  */
  NULL, /* Mailer entry.  */
  0, /* Not malloc()ed.  */
  NULL, /* No need for an owner.  */
  NULL, /* is_scheme method.  */
  NULL, /* get_mailbox method.  */
  NULL, /* get_mailer method.  */
  NULL /* get_folder method.  */
};

/* We export two functions: url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
record_t smtp_record = &_smtp_record;
mailer_entry_t smtp_entry = &_smtp_entry;

struct _smtp
{
  char *mailhost;
  char *localhost;
  bio_t bio;
  char *ptr;
  char *nl;
  char *buffer;
  size_t buflen;
  enum smtp_state
  {
    SMTP_NO_STATE, SMTP_OPEN, SMTP_GREETINGS, SMTP_EHLO, SMTP_EHLO_ACK,
    SMTP_HELO, SMTP_HELO_ACK, SMTP_QUIT, SMTP_QUIT_ACK, SMTP_ENV_FROM,
    SMTP_ENV_RCPT, SMTP_MAIL_FROM, SMTP_MAIL_FROM_ACK, SMTP_RCPT_TO,
    SMTP_RCPT_TO_ACK, SMTP_DATA, SMTP_DATA_ACK, SMTP_SEND, SMTP_SEND_ACK,
    SMTP_SEND_DOT
  } state;
  int extended;
  char *from;
  char *to;
  off_t offset;
  int dsn;
  message_t message;
};

typedef struct _smtp * smtp_t;

/* Usefull little Macros, since these are very repetitive.  */
#define CLEAR_STATE(smtp) \
 smtp->state = SMTP_NO_STATE

/* Clear the state and close the stream.  */
#define CHECK_ERROR_CLOSE(mailer, smtp, status) \
do \
  { \
     if (status != 0) \
       { \
          stream_close (mailer->stream); \
          CLEAR_STATE (smtp); \
          return status; \
       } \
  } \
while (0)

/* Clear the state.  */
#define CHECK_ERROR(smtp, status) \
do \
  { \
     if (status != 0) \
       { \
          CLEAR_STATE (smtp); \
          return status; \
       } \
  } \
while (0)

/* Clear the state for non recoverable error.  */
#define CHECK_EAGAIN(smtp, status) \
do \
  { \
    if (status != 0) \
      { \
         if (status != EAGAIN && status != EINPROGRESS && status != EINTR) \
           { \
             CLEAR_STATE (smtp); \
           } \
         return status; \
      } \
   }  \
while (0)

static void smtp_destroy (mailer_t);
static int smtp_open (mailer_t, int);
static int smtp_close (mailer_t);
static int smtp_send_message (mailer_t, const char *from, const char *rcpt,
			      int dsn, message_t);
static int smtp_writeline (smtp_t smtp, const char *format, ...);
static int smtp_readline (smtp_t);
static int smtp_read_ack (smtp_t);
static int smtp_write (smtp_t);

int
smtp_init (mailer_t mailer)
{
  smtp_t smtp;

  /* Allocate memory specific to smtp mailer.  */
  smtp = mailer->data = calloc (1, sizeof (*smtp));
  if (mailer->data == NULL)
    return ENOMEM;

  mailer->_init = smtp_init;
  mailer->_destroy = smtp_destroy;

  mailer->_open = smtp_open;
  mailer->_close = smtp_close;
  mailer->_send_message = smtp_send_message;

  return 0;
}

static void
smtp_destroy(mailer_t mailer)
{
  smtp_t smtp = mailer->data;
  smtp_close (mailer);
  if (smtp->mailhost)
    free (smtp->mailhost);
  if (smtp->localhost)
    free (smtp->localhost);
  if (smtp->bio)
    bio_destroy (&(smtp->bio));
  if (smtp->buffer)
    free (smtp->buffer);
  if (smtp->from)
    free (smtp->from);
  if (smtp->to)
    free (smtp->to);
  free (smtp);
  mailer->data = NULL;
}

static int
smtp_open (mailer_t mailer, int flags)
{
  smtp_t smtp = mailer->data;
  int status;
  long port;
  size_t buf_len = 0;

  /* Sanity checks.  */
  if (smtp == NULL)
    return EINVAL;

  mailer->flags = flags | MU_STREAM_SMTP;

  /* Fetch the mailer server name and the port in the url_t.  */
  if ((status = url_get_host (mailer->url, NULL, 0, &buf_len)) != 0
      || buf_len == 0 || (status = url_get_port (mailer->url, &port)) != 0)
    return status;

  switch (smtp->state)
    {
    case SMTP_NO_STATE:
      /* Get the mailhost.  */
      if (smtp->mailhost)
	{
	  free (smtp->mailhost);
	  smtp->mailhost = NULL;
	}
      smtp->mailhost = calloc (buf_len + 1, sizeof (char));
      if (smtp->mailhost == NULL)
	return ENOMEM;
      url_get_host (mailer->url, smtp->mailhost, buf_len + 1, NULL);

      if (smtp->localhost)
	{
	  free (smtp->localhost);
	  smtp->localhost = NULL;
	}
      /* Fetch our localhost name.  */
      buf_len = 64;
      do
	{
	  char *tmp;
	  errno = 0;
	  buf_len *= 2;    /* Initial guess */
	  tmp = realloc (smtp->localhost, buf_len);
	  if (tmp == NULL)
	    {
	      if (smtp->localhost)
		free (smtp->localhost);
	      smtp->localhost = NULL;
	      free (smtp->mailhost);
	      smtp->mailhost = NULL;
	      return ENOMEM;
	    }
	  smtp->localhost = tmp;
	}
      while (((status = gethostname(smtp->localhost, buf_len)) == 0
	      && !memchr (smtp->localhost, '\0', buf_len))
#ifdef ENAMETOOLONG
	     || errno == ENAMETOOLONG
#endif
	     );
      if (status != 0 && errno != 0)
	{
	  /* gethostname failed, abort.  */
	  free (smtp->localhost);
	  smtp->localhost = NULL;
	  free (smtp->mailhost);
	  smtp->mailhost = NULL;
	  return EINVAL;
	}

      /* Many SMTP servers prefer a FQDN.  */
      if (strchr (smtp->localhost, '.') == NULL)
	{
	  struct hostent *hp = gethostbyname (smtp->localhost);
	  if (hp == NULL)
	    {
	      /* Don't flag it as an error some SMTP servers can get the FQDN
		 by themselves even if the client is lying, probably
		 with getpeername().  */
	      // return EINVAL;
	    }
	  else
	    {
	      free (smtp->localhost);
	      smtp->localhost = strdup (hp->h_name);
	      if (smtp->localhost == NULL)
		{
		  free (smtp->mailhost);
		  smtp->mailhost = NULL;
		  return ENOMEM;
		}
	    }
	}

      /* allocate a working io buffer.  */
      if (smtp->buffer == NULL)
        {
          smtp->buflen = 255; /* Initial guess.  */
          smtp->buffer = malloc (smtp->buflen + 1);
          if (smtp->buffer == NULL)
            {
              CHECK_ERROR (smtp, ENOMEM);
            }
          smtp->ptr = smtp->buffer;
        }

      /* Create a TCP stack if one is not given.  */
      if (mailer->stream == NULL)
	{
	  status = tcp_stream_create (&(mailer->stream));
	  CHECK_ERROR (smtp, status);
	}
      status = bio_create (&(smtp->bio), mailer->stream);
      CHECK_ERROR (smtp, status);
      smtp->state = SMTP_OPEN;

    case SMTP_OPEN:
      status = stream_open (mailer->stream, smtp->mailhost, port,
			    mailer->flags);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_GREETINGS;

    case SMTP_GREETINGS:
      /* Swallow the greetings.  */
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);
      if (smtp->buffer[0] != '2')
	{
	  stream_close (mailer->stream);
	  return EACCES;
	}
      status = smtp_writeline (smtp, "EHLO %s\r\n", smtp->localhost);
      CHECK_ERROR (smtp, status);
      smtp->state = SMTP_EHLO;

    case SMTP_EHLO:
      /* We first try Extended SMTP.  */
      status = smtp_write (smtp);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_EHLO_ACK;

    case SMTP_EHLO_ACK:
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);
      if (smtp->buffer[0] != '2')
	{
	  smtp->extended = 0;
	  status = smtp_writeline (smtp, "HELO %s\r\n", smtp->localhost);
	  CHECK_ERROR (smtp, status);
	  smtp->state = SMTP_HELO;
	}
      else
	{
	  smtp->extended = 1;
	  break;
	}

    case SMTP_HELO:
      if (!smtp->extended)
	{
	  status = smtp_write (smtp);
	  CHECK_EAGAIN (smtp, status);
	}
      smtp->state = SMTP_HELO_ACK;

    case SMTP_HELO_ACK:
      if (!smtp->extended)
	{
	  status = smtp_read_ack (smtp);
	  CHECK_EAGAIN (smtp, status);
	  if (smtp->buffer[0] != '2')
	    {
	      stream_close (mailer->stream);
	      CLEAR_STATE (smtp);
	      return EACCES;
	    }
	}

    default:
      break;
    }
  smtp->state = SMTP_NO_STATE;
  return 0;
}

static int
smtp_close (mailer_t mailer)
{
  smtp_t smtp = mailer->data;
  int status;
  switch (smtp->state)
    {
    case SMTP_NO_STATE:
      status = smtp_writeline (smtp, "Quit\r\n");
      CHECK_ERROR (smtp, status);
      smtp->state = SMTP_QUIT;

    case SMTP_QUIT:
      status = smtp_write (smtp);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_QUIT_ACK;

    case SMTP_QUIT_ACK:
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);

    default:
      break;
    }
  return stream_close (mailer->stream);
}

static int
smtp_send_message(mailer_t mailer, const char *from, const char *rcpt,
		  int dsn, message_t msg)
{
  smtp_t smtp = mailer->data;
  int status;

  if (smtp == NULL || msg == NULL)
    return EINVAL;

  switch (smtp->state)
    {
    case SMTP_NO_STATE:
      smtp->dsn = dsn;
      smtp->state = SMTP_ENV_FROM;

    case SMTP_ENV_FROM:
      if (smtp->from)
	{
	  free (smtp->from);
	  smtp->from = NULL;
	}
      /* Try to fetch it from the header.  */
      if (from == NULL)
	{
	  header_t header;
	  size_t size;
	  message_get_header (msg, &header);
	  status = header_get_value (header, MU_HEADER_FROM, NULL, 0, &size);
	  if (status == EAGAIN)
	    return status;
	  /* If it's not in the header create one form the passwd.  */
	  if (status != 0)
	    {
	      struct passwd *pwd = getpwuid (getuid ());
	      /* Not in the passwd ???? We have a problem.  */
	      if (pwd == 0 || pwd->pw_name == NULL)
		{
		  size_t len = 10 + strlen (smtp->localhost) + 1;
		  smtp->from = calloc (len, sizeof (char));
		  if (smtp->from == NULL)
		    {
		      CHECK_ERROR (smtp, ENOMEM);
		    }
		  snprintf (smtp->from, len, "%d@%s", getuid(),
			    smtp->localhost);
		}
	      else
		{
		  smtp->from  = calloc (strlen (pwd->pw_name) + 1
					+ strlen (smtp->localhost) + 1,
					sizeof (char));
		  if (smtp->from == NULL)
		    {
		      CHECK_ERROR (smtp, ENOMEM);
		    }
		  sprintf(smtp->from, "%s@%s", pwd->pw_name, smtp->localhost);
		}
	    }
	  else
	    {
	      smtp->from = calloc (size + 1, sizeof (char));
	      if (smtp->from == NULL)
		{
		  CHECK_ERROR (smtp, ENOMEM);
		}
	      status = header_get_value (header, MU_HEADER_FROM, smtp->from,
					 size + 1, NULL);
	      CHECK_EAGAIN (smtp, status);
	    }
	}
      else
	{
	  smtp->from = strdup (from);
	  if (smtp->from == NULL)
	    {
	      CHECK_ERROR (smtp, ENOMEM);
	    }
	}
      /* Check if a Fully Qualified Name, some smtp servers
	 notably sendmail insists on it, for good reasons.  */
      if (strchr (smtp->from, '@') == NULL)
	{
	  char *tmp;
	  tmp = malloc (strlen (smtp->from) + 1 +strlen (smtp->localhost) + 1);
	  if (tmp == NULL)
	    {
	      free (smtp->from);
	      smtp->from = NULL;
	      CHECK_ERROR (smtp, ENOMEM);
	    }
	  sprintf (tmp, "%s@%s", smtp->from, smtp->localhost);
	  free (smtp->from);
	  smtp->from = tmp;
	}
      smtp->state = SMTP_ENV_RCPT;

    case SMTP_ENV_RCPT:
      if (smtp->to)
	{
	  free (smtp->to);
	  smtp->to = NULL;
	}
      if (rcpt == NULL)
	{
	  header_t header;
	  size_t size;
	  message_get_header (msg, &header);
	  status = header_get_value (header, MU_HEADER_TO, NULL, 0, &size);
	  CHECK_EAGAIN (smtp, status);
	  smtp->to = calloc (size + 1, sizeof (char));
	  if (smtp->to == NULL)
	    {
	      CHECK_ERROR (smtp, ENOMEM);
	    }
	  status = header_get_value (header, MU_HEADER_TO, smtp->to,
				     size + 1, NULL);
	  CHECK_EAGAIN (smtp, status);
	}
      else
	{
	  smtp->to = strdup (rcpt);
	  if (smtp->to == NULL)
	    {
	      CHECK_ERROR (smtp, ENOMEM);
	    }
	}

      status = smtp_writeline (smtp, "MAIL FROM: %s\r\n", smtp->from);
      free (smtp->from);
      smtp->from = NULL;
      CHECK_ERROR (smtp, status);
      smtp->state = SMTP_MAIL_FROM;

    case SMTP_MAIL_FROM:
      status = smtp_write (smtp);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_MAIL_FROM_ACK;

    case SMTP_MAIL_FROM_ACK:
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);
      if (smtp->buffer[0] != '2')
	{
	  stream_close (mailer->stream);
	  CLEAR_STATE (smtp);
	  return EACCES;
	}
      /* We use a goto, since we may have multiple recipients,
	 we come back here and doit all over again ... Not pretty.  */
    RCPT_TO:
      {
	char *buf;
	size_t len = strlen (smtp->to) + 1;
	buf = calloc (len, sizeof (char));
	if (buf == NULL)
	  {
	    CHECK_ERROR (smtp, ENOMEM);
	  }
	if (parseaddr (smtp->to, buf, len) != 0)
	  {
	    free (buf);
	    CHECK_ERROR (smtp, EINVAL);
	  }
	status = smtp_writeline (smtp, "RCPT TO: %s\r\n", buf);
	free (buf);
	CHECK_ERROR (smtp, status);
	smtp->state = SMTP_RCPT_TO;
      }

    case SMTP_RCPT_TO:
      status = smtp_write (smtp);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_RCPT_TO_ACK;

    case SMTP_RCPT_TO_ACK:
      {
	char *p;
	status = smtp_read_ack (smtp);
	CHECK_EAGAIN (smtp, status);
	if (smtp->buffer[0] != '2')
	  {
	    stream_close (mailer->stream);
	    CLEAR_STATE (smtp);
	    return EACCES;
	  }
	/* Do we have multiple recipients ?  */
	p = strchr (smtp->to, ',');
	if (p != NULL)
	  {
	    char *tmp = smtp->to;
	    smtp->to = strdup (p++);
	    if (smtp->to == NULL)
	      {
		free (tmp);
		CHECK_ERROR (smtp, ENOMEM);
	      }
	    free (tmp);
	    goto RCPT_TO;
	  }
	/* We are done with the rcpt.  */
	free (smtp->to);
	smtp->to = NULL;
	status = smtp_writeline (smtp, "DATA\r\n");
	CHECK_ERROR (smtp, status);
	smtp->state = SMTP_DATA;
      }

    case SMTP_DATA:
      status = smtp_write (smtp);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_DATA_ACK;

    case SMTP_DATA_ACK:
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);
      if (smtp->buffer[0] != '3')
	{
	  stream_close (mailer->stream);
	  CLEAR_STATE (smtp);
	  return EACCES;
	}
      smtp->offset = 0;
      smtp->state = SMTP_SEND;

    case SMTP_SEND:
      {
	stream_t stream;
	size_t n = 0;
	char data[256] = "";
	/* We may be here after an EAGAIN so check if we have something
	   in the buffer and flush it.  */
	status = smtp_write (smtp);
	CHECK_EAGAIN (smtp, status);
	message_get_stream (msg, &stream);
	while ((status = stream_readline (stream, data, sizeof (data) - 1,
					  smtp->offset, &n)) == 0 && n > 0)
	  {
	    if (data [n - 1] == '\n')
	      data [n -1] = '\0';
	    if (data[0] == '.')
	      {
		status = smtp_writeline (smtp, ".%s\r\n", data);
		CHECK_ERROR (smtp, status);
	      }
	    else
	      {
		status = smtp_writeline (smtp, "%s\r\n", data);
		CHECK_ERROR (smtp, status);
	      }
	    smtp->offset += n;
	    status = smtp_write (smtp);
	    CHECK_EAGAIN (smtp, status);
	  }
	smtp->offset = 0;
	status = smtp_writeline (smtp, ".\r\n");
	CHECK_ERROR (smtp, status);
	smtp->state = SMTP_SEND_DOT;
      }

      case SMTP_SEND_DOT:
	status = smtp_write (smtp);
	CHECK_EAGAIN (smtp, status);
	smtp->state = SMTP_SEND_ACK;

    case SMTP_SEND_ACK:
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);
      if (smtp->buffer[0] != '2')
	{
	  stream_close (mailer->stream);
	  CLEAR_STATE (smtp);
	  return EACCES;
	}

    default:
      break;
    }
  CLEAR_STATE (smtp);
  return 0;
}

static int
smtp_writeline (smtp_t smtp, const char *format, ...)
{
  int len;
  va_list ap;

  va_start(ap, format);
  do
    {
      len = vsnprintf (smtp->buffer, smtp->buflen - 1, format, ap);
      if (len >= (int)smtp->buflen)
        {
          smtp->buflen *= 2;
          smtp->buffer = realloc (smtp->buffer, smtp->buflen);
          if (smtp->buffer == NULL)
            return ENOMEM;
        }
    }
  while (len > (int)smtp->buflen);
  va_end(ap);
  smtp->ptr = smtp->buffer + len;
  return 0;
}

static int
smtp_write (smtp_t smtp)
{
  int status = 0;
  size_t len;
  if (smtp->ptr > smtp->buffer)
    {
      len = smtp->ptr - smtp->buffer;
      status = bio_write (smtp->bio, smtp->buffer, len, &len);
      if (status == 0)
        {
          memmove (smtp->buffer, smtp->buffer + len, len);
          smtp->ptr -= len;
        }
    }
  else
    {
      smtp->ptr = smtp->buffer;
      len = 0;
    }
  return status;
}

static int
smtp_read_ack (smtp_t smtp)
{
  int status;
  int multi;

  do
    {
      multi = 0;
      status = smtp_readline (smtp);
      if ((smtp->ptr - smtp->buffer) > 4
	  && smtp->buffer[3] == '-')
	multi = 1;
      if (status == 0)
	smtp->ptr = smtp->buffer;
    }
  while (multi && status == 0);

  if (status == 0)
    smtp->ptr = smtp->buffer;
  return status;
}

/* Read a complete line form the pop server. Transform CRLF to LF,
   put a null in the buffer when done.  */
static int
smtp_readline (smtp_t smtp)
{
  size_t n = 0;
  size_t total = smtp->ptr - smtp->buffer;
  int status;

  /* Must get a full line before bailing out.  */
  do
    {
      status = bio_readline (smtp->bio, smtp->buffer + total,
                             smtp->buflen - total, &n);
      if (status != 0)
        return status;

      total += n;
      smtp->nl = memchr (smtp->buffer, '\n', total);
      if (smtp->nl == NULL)  /* Do we have a full line.  */
        {
          /* Allocate a bigger buffer ?  */
          if (total >= smtp->buflen -1)
            {
              smtp->buflen *= 2;
              smtp->buffer = realloc (smtp->buffer, smtp->buflen + 1);
              if (smtp->buffer == NULL)
                return ENOMEM;
            }
        }
      smtp->ptr = smtp->buffer + total;
    }
  while (smtp->nl == NULL);

  /* \r\n --> \n\0  */
  if (smtp->nl > smtp->buffer)
    {
      *(smtp->nl - 1) = '\n';
      *(smtp->nl) = '\0';
      smtp->ptr = smtp->nl;
    }
  return 0;
}
