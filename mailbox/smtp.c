/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/** @file smtp.c
@brief an SMTP mailer
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_SMTP

#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/message.h>
#include <mailutils/mutil.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>

#include <mailer0.h>
#include <registrar0.h>

static struct _record _smtp_record = {
  MU_SMTP_SCHEME,
  _url_smtp_init,		/* url init.  */
  NULL,				/* Mailbox init.  */
  &_mailer_smtp_init,		/* Mailer init.  */
  NULL,				/* Folder init.  */
  NULL,				/* No need for a back pointer.  */
  NULL,				/* _is_scheme method.  */
  NULL,				/* _get_url method.  */
  NULL,				/* _get_mailbox method.  */
  NULL,				/* _get_mailer method.  */
  NULL				/* _get_folder method.  */
};
/* We export : url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
record_t smtp_record = &_smtp_record;

struct _smtp
{
  mailer_t mailer;
  char *mailhost;
  char *localhost;

  /* IO buffering. */
  char *buffer;			/* Must be freed. */
  size_t buflen;

  char *ptr;
  char *nl;
  off_t s_offset;

  enum smtp_state
  {
    SMTP_NO_STATE, SMTP_OPEN, SMTP_GREETINGS, SMTP_EHLO, SMTP_EHLO_ACK,
    SMTP_HELO, SMTP_HELO_ACK, SMTP_QUIT, SMTP_QUIT_ACK, SMTP_ENV_FROM,
    SMTP_ENV_RCPT, SMTP_MAIL_FROM, SMTP_MAIL_FROM_ACK, SMTP_RCPT_TO,
    SMTP_RCPT_TO_ACK, SMTP_DATA, SMTP_DATA_ACK, SMTP_SEND, SMTP_SEND_ACK,
    SMTP_SEND_DOT
  }
  state;

  int extended;

  char *mail_from;
  address_t rcpt_to;		/* Destroy this if not the same as argto below. */
  address_t rcpt_bcc;
  size_t rcpt_to_count;
  size_t rcpt_bcc_count;
  size_t rcpt_index;
  size_t rcpt_count;
  int bccing;
  message_t msg;		/* Destroy this if not same argmsg. */

  off_t offset;

  /* The mailer_send_message() args. */
  message_t argmsg;
  address_t argfrom;
  address_t argto;
};

typedef struct _smtp *smtp_t;

static void smtp_destroy (mailer_t);
static int smtp_open (mailer_t, int);
static int smtp_close (mailer_t);
static int smtp_send_message (mailer_t, message_t, address_t, address_t);
static int smtp_writeline (smtp_t smtp, const char *format, ...);
static int smtp_readline (smtp_t);
static int smtp_read_ack (smtp_t);
static int smtp_write (smtp_t);

static int _smtp_set_from (smtp_t, message_t, address_t);
static int _smtp_set_rcpt (smtp_t, message_t, address_t);

/* Useful little macros, since these are very repetitive. */

static void
CLEAR_STATE (smtp_t smtp)
{
  smtp->ptr = NULL;
  smtp->nl = NULL;
  smtp->s_offset = 0;

  smtp->state = SMTP_NO_STATE;

  smtp->extended = 0;

  if (smtp->mail_from)
    {
      free (smtp->mail_from);
      smtp->mail_from = NULL;
    }

  if (smtp->rcpt_to != smtp->argto)
    address_destroy (&smtp->rcpt_to);

  smtp->rcpt_to = NULL;

  address_destroy (&smtp->rcpt_bcc);

  smtp->rcpt_to_count = 0;
  smtp->rcpt_bcc_count = 0;
  smtp->rcpt_index = 0;
  smtp->rcpt_count = 0;
  smtp->bccing = 0;

  if (smtp->msg != smtp->argmsg)
    message_destroy (&smtp->msg, NULL);

  smtp->msg = NULL;

  smtp->offset = 0;

  smtp->argmsg = NULL;
  smtp->argfrom = NULL;
  smtp->argto = NULL;
}

/* If we are resuming, we should be resuming the SAME operation
   as that which is ongoing. Check this. */
static int
smtp_check_send_resumption (smtp_t smtp,
			    message_t msg, address_t from, address_t to)
{
  if (smtp->state == SMTP_NO_STATE)
    return 0;

  /* FIXME: state should be one of the "send" states if its not
     "no state" */
  if (msg != smtp->argmsg)
    return MU_ERR_BAD_RESUMPTION;

  if (from != smtp->argfrom)
    return MU_ERR_BAD_RESUMPTION;

  if (to != smtp->argto)
    return MU_ERR_BAD_RESUMPTION;

  return 0;
}

#define CHECK_SEND_RESUME(smtp, msg, from, to) \
do { \
  if((status = smtp_check_send_resumption(smtp, msg, from, to)) != 0) \
    return status; \
} while (0)

/* Clear the state and close the stream. */
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

/* Clear the state. */
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

int
_mailer_smtp_init (mailer_t mailer)
{
  smtp_t smtp;

  /* Allocate memory specific to smtp mailer.  */
  smtp = mailer->data = calloc (1, sizeof (*smtp));
  if (mailer->data == NULL)
    return ENOMEM;

  smtp->mailer = mailer;	/* Back pointer.  */
  smtp->state = SMTP_NO_STATE;

  mailer->_destroy = smtp_destroy;
  mailer->_open = smtp_open;
  mailer->_close = smtp_close;
  mailer->_send_message = smtp_send_message;

  /* Set our properties.  */
  {
    property_t property = NULL;
    mailer_get_property (mailer, &property);
    property_set_value (property, "TYPE", "SMTP", 1);
  }

  return 0;
}

static void
smtp_destroy (mailer_t mailer)
{
  smtp_t smtp = mailer->data;

  CLEAR_STATE (smtp);

  /* Not our responsability to close.  */

  if (smtp->mailhost)
    free (smtp->mailhost);
  if (smtp->localhost)
    free (smtp->localhost);
  if (smtp->buffer)
    free (smtp->buffer);

  free (smtp);

  mailer->data = NULL;
}

/** Open an SMTP mailer.
An SMTP mailer must be opened before any messages can be sent.
@param mailer the mailer created by smtp_create()
@param flags the mailer flags
*/
static int
smtp_open (mailer_t mailer, int flags)
{
  smtp_t smtp = mailer->data;
  int status;
  long port;
  size_t buf_len = 0;

  /* Sanity checks.  */
  assert (smtp);

  mailer->flags = flags;

  /* Fetch the mailer server name and the port in the url_t.  */
  if ((status = url_get_host (mailer->url, NULL, 0, &buf_len)) != 0
      || buf_len == 0 || (status = url_get_port (mailer->url, &port)) != 0)
    return status;

  switch (smtp->state)
    {
    case SMTP_NO_STATE:
/* Set up the mailer, open the stream, etc. */
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
      /* Fetch our local host name.  */

      status = mu_get_host_name (&smtp->localhost);

      if (status != 0)
	{
	  /* gethostname failed, abort.  */
	  free (smtp->mailhost);
	  smtp->mailhost = NULL;
	  return status;
	}

      /* allocate a working io buffer.  */
      if (smtp->buffer == NULL)
	{
	  smtp->buflen = 512;	/* Initial guess.  */
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
	  status =
	    tcp_stream_create (&mailer->stream, smtp->mailhost, port,
			       mailer->flags);
	  CHECK_ERROR (smtp, status);
	  stream_setbufsiz (mailer->stream, BUFSIZ);
	}
      CHECK_ERROR (smtp, status);
      smtp->state = SMTP_OPEN;

    case SMTP_OPEN:
      MAILER_DEBUG2 (mailer, MU_DEBUG_PROT, "smtp_open (host: %s port: %d)\n",
		     smtp->mailhost, port);
      status = stream_open (mailer->stream);
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
      if (!smtp->extended)	/* FIXME: this will always be false! */
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

  CLEAR_STATE (smtp);

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
      status = smtp_writeline (smtp, "QUIT\r\n");
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
message_set_header_value (message_t msg, const char *field, const char *value)
{
  int status = 0;
  header_t hdr = NULL;

  if ((status = message_get_header (msg, &hdr)))
    return status;

  if ((status = header_set_value (hdr, field, value, 1)))
    return status;

  return status;
}

static int
message_has_bcc (message_t msg)
{
  int status;
  header_t header = NULL;
  size_t bccsz = 0;

  if ((status = message_get_header (msg, &header)))
    return status;

  status = header_get_value (header, MU_HEADER_BCC, NULL, 0, &bccsz);

  /* ENOENT, or there was a Bcc: field. */
  return status == ENOENT ? 0 : 1;
}

/*

The smtp mailer doesn't deal with mail like:

To: public@com, pub2@com
Bcc: hidden@there, two@there

It just sends the message to all the addresses, making the
"blind" cc not particularly blind.

The correct algorithm is

- open smtp connection
- look as msg, figure out addrto&cc, and addrbcc
- deliver to the to & cc addresses:
  - if there are bcc addrs, remove the bcc field
  - send the message to to & cc addrs:
  mail from: me
  rcpt to: public@com
  rcpt to: pub2@com
  data
  ...

- deliver to the bcc addrs:

  for a in (bccaddrs)
  do
    - add header field to msg,  bcc: $a
    - send the msg:
    mail from: me
    rcpt to: $a
    data
    ...
  done

- quit smtp connection

*/

static int
smtp_send_message (mailer_t mailer, message_t argmsg, address_t argfrom,
		   address_t argto)
{
  smtp_t smtp = NULL;
  int status;

  if (mailer == NULL)
    return EINVAL;

  smtp = mailer->data;
  assert (smtp);

  CHECK_SEND_RESUME (smtp, argmsg, argfrom, argto);

  switch (smtp->state)
    {
    case SMTP_NO_STATE:
      if (argmsg == NULL)
	return EINVAL;

      smtp->argmsg = smtp->msg = argmsg;
      smtp->argfrom = argfrom;
      smtp->argto = argto;

      status = _smtp_set_from (smtp, smtp->argmsg, smtp->argfrom);
      CHECK_ERROR (smtp, status);

      status = _smtp_set_rcpt (smtp, smtp->argmsg, smtp->argto);
      CHECK_ERROR (smtp, status);

      /* Clear the Bcc: field if we found one. */
      if (message_has_bcc (smtp->argmsg))
	{
	  smtp->msg = NULL;
	  status = message_create_copy (&smtp->msg, smtp->argmsg);
	  CHECK_ERROR (smtp, status);

	  status = message_set_header_value (smtp->msg, MU_HEADER_BCC, NULL);
	  CHECK_ERROR (smtp, status);
	}

      /* Begin bccing if there are not To: recipients. */
      if (smtp->rcpt_to_count == 0)
	smtp->bccing = 1;

      smtp->rcpt_index = 1;

      smtp->state = SMTP_ENV_FROM;

    case SMTP_ENV_FROM:
    ENV_FROM:
      {
	status = smtp_writeline (smtp, "MAIL FROM: <%s>\r\n", smtp->mail_from);
	CHECK_ERROR (smtp, status);
	smtp->state = SMTP_MAIL_FROM;
      }

      /* We use a goto, since we may have multiple messages,
         we come back here and doit all over again ... Not pretty.  */
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
         we come back here and do it all over again ... Not pretty. */
    case SMTP_ENV_RCPT:
    ENV_RCPT:
      {
	address_t addr = smtp->rcpt_to;
	char *to = NULL;

	if (smtp->bccing)
	  addr = smtp->rcpt_bcc;
	status = address_aget_email (addr, smtp->rcpt_index, &to);

	CHECK_ERROR (smtp, status);

	/* Add the Bcc: field back in for recipient. */
	if (smtp->bccing)
	  {
	    status = message_set_header_value (smtp->msg, MU_HEADER_BCC, to);
	    CHECK_ERROR (smtp, status);
	  }

	status = smtp_writeline (smtp, "RCPT TO: <%s>\r\n", to);

	free (to);

	CHECK_ERROR (smtp, status);

	smtp->state = SMTP_RCPT_TO;
	smtp->rcpt_index++;
      }

    case SMTP_RCPT_TO:
      status = smtp_write (smtp);
      CHECK_EAGAIN (smtp, status);
      smtp->state = SMTP_RCPT_TO_ACK;

    case SMTP_RCPT_TO_ACK:
      status = smtp_read_ack (smtp);
      CHECK_EAGAIN (smtp, status);
      if (smtp->buffer[0] != '2')
	{
	  stream_close (mailer->stream);
	  CLEAR_STATE (smtp);
	  return MU_ERR_SMTP_RCPT_FAILED;
	}
      /* Redo the receipt sequence for every To: and Cc: recipient. */
      if (!smtp->bccing && smtp->rcpt_index <= smtp->rcpt_to_count)
	goto ENV_RCPT;

      /* We are done with the rcpt. */
      status = smtp_writeline (smtp, "DATA\r\n");
      CHECK_ERROR (smtp, status);
      smtp->state = SMTP_DATA;

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

      if ((smtp->mailer->flags & MAILER_FLAG_DEBUG_DATA) == 0)
	MAILER_DEBUG0 (smtp->mailer, MU_DEBUG_PROT, "> (data...)\n");

    case SMTP_SEND:
      {
	stream_t stream;
	size_t n = 0;
	char data[256] = "";
	header_t hdr;
	body_t body;
	int found_nl;
	
	/* We may be here after an EAGAIN so check if we have something
	   in the buffer and flush it.  */
	status = smtp_write (smtp);
	CHECK_EAGAIN (smtp, status);

	message_get_header (smtp->msg, &hdr);
	header_get_stream (hdr, &stream);
	while ((status = stream_readline (stream, data, sizeof (data),
					  smtp->offset, &n)) == 0 && n > 0)
	  {
	    int nl;
	    
	    found_nl = (n == 1 && data[0] == '\n');
	    if ((nl = (data[n - 1] == '\n')))
	      data[n - 1] = '\0';
	    if (data[0] == '.')
	      {
		status = smtp_writeline (smtp, ".%s", data);
		CHECK_ERROR (smtp, status);
	      }
	    else if (strncasecmp (data, MU_HEADER_FCC,
				  sizeof (MU_HEADER_FCC) - 1))
	      {
		status = smtp_writeline (smtp, "%s", data);
		CHECK_ERROR (smtp, status);
		status = smtp_write (smtp);
		CHECK_EAGAIN (smtp, status);
	      }
	    if (nl)
	      {
		status = smtp_writeline (smtp, "\r\n");
		CHECK_ERROR (smtp, status);
		status = smtp_write (smtp);
		CHECK_EAGAIN (smtp, status);
	      }
	    smtp->offset += n;
	  }
	
	if (!found_nl)
	  {
	    status = smtp_writeline (smtp, "\r\n");
	    CHECK_ERROR (smtp, status);
	    status = smtp_write (smtp);
	    CHECK_EAGAIN (smtp, status);
	  }
	
	message_get_body (smtp->msg, &body);
	body_get_stream (body, &stream);
	smtp->offset = 0;
	while ((status = stream_readline (stream, data, sizeof (data) - 1,
					  smtp->offset, &n)) == 0 && n > 0)
	  {
	    if (data[n - 1] == '\n')
	      data[n - 1] = '\0';
	    if (data[0] == '.')
	      status = smtp_writeline (smtp, ".%s\r\n", data);
	    else
	      status = smtp_writeline (smtp, "%s\r\n", data);
	    CHECK_ERROR (smtp, status);
	    status = smtp_write (smtp);
	    CHECK_EAGAIN (smtp, status);
	    smtp->offset += n;
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

      /* Decide whether we need to loop again, to deliver to Bcc:
         recipients. */
      if (!smtp->bccing)
	{
	  smtp->bccing = 1;
	  smtp->rcpt_index = 1;
	}
      if (smtp->rcpt_index <= smtp->rcpt_bcc_count)
	goto ENV_FROM;

      observable_notify (mailer->observable, MU_EVT_MAILER_MESSAGE_SENT);

    default:
      break;
    }
  CLEAR_STATE (smtp);
  return 0;
}

static int
_smtp_set_from (smtp_t smtp, message_t msg, address_t from)
{
  int status = 0;
  char *mail_from;
  header_t header = NULL;

  /* Get MAIL_FROM from FROM, the message, or the environment. */
  if (from)
    {
      /* Use the specified address_t. */
      if ((status = mailer_check_from (from)) != 0)
	{
	  MAILER_DEBUG0 (smtp->mailer, MU_DEBUG_ERROR,
			 "mailer_send_message(): explicit from not valid\n");
	  return status;
	}

      if ((status = address_aget_email (from, 1, &mail_from)) != 0)
	return status;
    }
  else
    {
      char *from_hdr = NULL;

      if ((status = message_get_header (msg, &header)) != 0)
	return status;

      status = header_aget_value (header, MU_HEADER_FROM, &from_hdr);

      switch (status)
	{
	default:
	  return status;

	  /* Use the From: header. */
	case 0:
	  {
	    address_t fromaddr = NULL;

	    MAILER_DEBUG1 (smtp->mailer, MU_DEBUG_TRACE,
			   "mailer_send_message(): using From: %s\n",
			   from_hdr);

	    if ((status = address_create (&fromaddr, from_hdr)) != 0)
	      {
		free (from_hdr);
		return status;
	      }
	    if ((status = mailer_check_from (fromaddr)) != 0)
	      {
		free (from_hdr);
		address_destroy (&fromaddr);
		MAILER_DEBUG1 (smtp->mailer, MU_DEBUG_ERROR,
			       "mailer_send_message(): from field %s not valid\n",
			       from_hdr);
		return status;
	      }
	    if ((status = address_aget_email (fromaddr, 1, &mail_from)) != 0)
	      {
		free (from_hdr);
		address_destroy (&fromaddr);
		return status;
	      }
	    free (from_hdr);
	    address_destroy (&fromaddr);
	  }
	  break;

	case ENOENT:
	  /* Use the environment. */
	  mail_from = mu_get_user_email (NULL);

	  if (mail_from)
	    {
	      MAILER_DEBUG1 (smtp->mailer, MU_DEBUG_TRACE,
			     "mailer_send_message(): using user's address: %s\n",
			     mail_from);
	    }
	  else
	    {
	      MAILER_DEBUG0 (smtp->mailer, MU_DEBUG_TRACE,
			     "mailer_send_message(): no user's address, failing\n");
	    }

	  if (!mail_from)
	    return errno;

	  status = 0;

	  /* FIXME: should we add the From: header? */

	  break;

	}
    }

  assert (mail_from);

  smtp->mail_from = mail_from;

  return status;
}

int
smtp_address_add (address_t *paddr, const char *value)
{
  address_t addr = NULL;
  int status;

  status = address_create (&addr, value);
  if (status)
    return status;
  status = address_union (paddr, addr);
  address_destroy (&addr);
  return status;
}

static int
_smtp_property_is_set (smtp_t smtp, const char *name)
{
  property_t property = NULL;

  mailer_get_property (smtp->mailer, &property);
  return property_is_set (property, name);
}

static int
_smtp_set_rcpt (smtp_t smtp, message_t msg, address_t to)
{
  int status = 0;
  header_t header = NULL;
  char *value;

  /* Get RCPT_TO from TO, or the message. */

  if (to)
    {
      /* Use the specified address_t. */
      if ((status = mailer_check_to (to)) != 0)
	{
	  MAILER_DEBUG0 (smtp->mailer, MU_DEBUG_ERROR,
			 "mailer_send_message(): explicit to not valid\n");
	  return status;
	}
      smtp->rcpt_to = to;
      address_get_count (smtp->rcpt_to, &smtp->rcpt_to_count);

      if (status)
	return status;
    }

  if (!to || _smtp_property_is_set (smtp, "READ_RECIPIENTS"))
    {
      if ((status = message_get_header (msg, &header)))
	return status;

      status = header_aget_value (header, MU_HEADER_TO, &value);

      if (status == 0)
	{
	  smtp_address_add (&smtp->rcpt_to, value);
	  free (value);
	}
      else if (status != ENOENT)
	goto end;

      status = header_aget_value (header, MU_HEADER_CC, &value);

      if (status == 0)
	{
	  smtp_address_add (&smtp->rcpt_to, value);
	  free (value);
	}
      else if (status != ENOENT)
	goto end;

      status = header_aget_value (header, MU_HEADER_BCC, &value);
      if (status == 0)
	{
	  smtp_address_add (&smtp->rcpt_bcc, value);
	  free (value);
	}
      else if (status != ENOENT)
	goto end;

      /* If to or bcc is present, the must be OK. */
      if (smtp->rcpt_to && (status = mailer_check_to (smtp->rcpt_to)))
	goto end;

      if (smtp->rcpt_bcc && (status = mailer_check_to (smtp->rcpt_bcc)))
	goto end;
    }
  
end:

  if (status)
    {
      address_destroy (&smtp->rcpt_to);
      address_destroy (&smtp->rcpt_bcc);
    }
  else
    {
      if (smtp->rcpt_to)
	address_get_count (smtp->rcpt_to, &smtp->rcpt_to_count);

      if (smtp->rcpt_bcc)
	address_get_count (smtp->rcpt_bcc, &smtp->rcpt_bcc_count);

      if (smtp->rcpt_to_count + smtp->rcpt_bcc_count == 0)
	status = MU_ERR_MAILER_NO_RCPT_TO;
    }

  return status;
}

/* C99 says that a conforming implementations of snprintf ()
   should return the number of char that would have been call
   but many GNU/Linux && BSD implementations return -1 on error.
   Worse QNX/Neutrino actually does not put the terminal
   null char.  So let's try to cope.  */
static int
smtp_writeline (smtp_t smtp, const char *format, ...)
{
  int len;
  va_list ap;
  int done = 1;

  va_start (ap, format);
  do
    {
      len = vsnprintf (smtp->buffer, smtp->buflen - 1, format, ap);
      if (len < 0 || (len >= (int) smtp->buflen)
	  || !memchr (smtp->buffer, '\0', len + 1))
	{
	  char *buffer = NULL;
	  size_t buflen = smtp->buflen * 2;
	  buffer = realloc (smtp->buffer, buflen);
	  if (smtp->buffer == NULL)
	    return ENOMEM;
	  smtp->buffer = buffer;
	  smtp->buflen = buflen;
	  done = 0;
	}
      else
	done = 1;
    }
  while (!done);

  va_end (ap);

  smtp->ptr = smtp->buffer + len;

  while (len > 0 && isspace (smtp->buffer[len - 1]))
    len--;

  if ((smtp->state != SMTP_SEND && smtp->state != SMTP_SEND_DOT)
      || smtp->mailer->flags & MAILER_FLAG_DEBUG_DATA)
    {
      MAILER_DEBUG2 (smtp->mailer, MU_DEBUG_PROT, "> %.*s\n", len,
		     smtp->buffer);
    }

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
      status = stream_write (smtp->mailer->stream, smtp->buffer, len,
			     0, &len);
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
      if ((smtp->ptr - smtp->buffer) > 4 && smtp->buffer[3] == '-')
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
      status = stream_readline (smtp->mailer->stream, smtp->buffer + total,
				smtp->buflen - total, smtp->s_offset, &n);
      if (status != 0)
	return status;

      /* Server went away, consider this like an error.  */
      if (n == 0)
	return EIO;

      total += n;
      smtp->s_offset += n;
      smtp->nl = memchr (smtp->buffer, '\n', total);
      if (smtp->nl == NULL)	/* Do we have a full line.  */
	{
	  /* Allocate a bigger buffer ?  */
	  if (total >= smtp->buflen - 1)
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

  MAILER_DEBUG1 (smtp->mailer, MU_DEBUG_PROT, "< %s", smtp->buffer);

  return 0;
}

#else
#include <stdio.h>
#include <registrar0.h>
record_t smtp_record = NULL;
#endif
