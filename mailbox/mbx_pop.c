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

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include <mailutils/stream.h>
#include <mailutils/body.h>
#include <mailutils/message.h>
#include <mailutils/header.h>
#include <mailutils/attribute.h>
#include <mailutils/url.h>
#include <mailutils/auth.h>
#include <mailbox0.h>
#include <registrar0.h>
#include <bio.h>

static int pop_init (mailbox_t);

static struct mailbox_entry _pop_entry =
{
  url_pop_init, pop_init
};
static struct _record _pop_record =
{
  MU_POP_SCHEME,
  &_pop_entry, /* Mailbox entry.  */
  NULL, /* Mailer entry.  */
  0, /* Not malloc()ed.  */
  NULL, /* No need for an owner.  */
  NULL, /* is_scheme method.  */
  NULL, /* get_mailbox method.  */
  NULL /* get_mailer method.  */
};

/* We export two functions: url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
record_t pop_record = &_pop_record;
mailbox_entry_t pop_entry = &_pop_entry;

/* Advance declarations.  */
struct _pop_data;
struct _pop_message;

typedef struct _pop_data * pop_data_t;
typedef struct _pop_message * pop_message_t;

/* The different possible states of a Pop client, Note that POP3 is not
   reentrant. It is only one channel.  */
enum pop_state
{
  POP_NO_STATE, POP_STATE_DONE,
  POP_OPEN_CONNECTION,
  POP_GREETINGS,
  POP_APOP, POP_APOP_ACK,
  POP_DELE, POP_DELE_ACK,
  POP_LIST, POP_LIST_ACK, POP_LIST_RX,
  POP_QUIT, POP_QUIT_ACK,
  POP_NOOP, POP_NOOP_ACK,
  POP_RETR, POP_RETR_ACK, POP_RETR_RX_HDR, POP_RETR_RX_BODY,
  POP_RSET, POP_RSET_ACK,
  POP_STAT, POP_STAT_ACK,
  POP_TOP,  POP_TOP_ACK,  POP_TOP_RX,
  POP_UIDL, POP_UIDL_ACK,
  POP_AUTH, POP_AUTH_DONE,
  POP_AUTH_USER, POP_AUTH_USER_ACK,
  POP_AUTH_PASS, POP_AUTH_PASS_ACK
};

static void pop_destroy (mailbox_t);

/*  Functions/Methods that implements the mailbox_t API.  */
static int pop_open (mailbox_t, int);
static int pop_close (mailbox_t);
static int pop_get_message (mailbox_t, size_t, message_t *);
static int pop_messages_count (mailbox_t, size_t *);
static int pop_expunge (mailbox_t);
static int pop_scan (mailbox_t, size_t, size_t *);
static int pop_is_updated (mailbox_t);

/* The implementation of message_t */
static int pop_user (authority_t);
static int pop_size (mailbox_t, off_t *);
static int pop_header_read (stream_t, char *, size_t, off_t, size_t *);
static int pop_header_fd (stream_t, int *);
static int pop_body_fd (stream_t, int *);
static int pop_body_size (body_t, size_t *);
static int pop_body_lines (body_t, size_t *);
static int pop_body_read (stream_t, char *, size_t, off_t, size_t *);
static int pop_message_read (stream_t, char *, size_t, off_t, size_t *);
static int pop_message_size (message_t, size_t *);
static int pop_message_fd (stream_t, int *);
static int pop_top (stream_t, char *, size_t, off_t, size_t *);
static int pop_retr (pop_message_t, char *, size_t, off_t, size_t *);
static int pop_get_fd (pop_message_t, int *);
static int pop_attr_flags (attribute_t, int *);
static int pop_uidl (message_t, char *, size_t, size_t *);
static int fill_buffer (pop_data_t, char *, size_t);
static int pop_readline (pop_data_t);
static int pop_read_ack (pop_data_t);
static int pop_writeline (pop_data_t, const char *, ...);
static int pop_write (pop_data_t);

/* This structure holds the info for a pop_get_message(). The pop_message_t
   type, will serve as the owner of the message_t and contains the command to
   send to "RETR"eive the specify message.  The problem comes
   from the header.  If the  POP server supports TOP, we can cleanly fetch
   the header.  But otherwise we use the clumsy approach. .i.e for the header
   we read 'til ^\n then discard the rest, for the body we read after ^\n and
   discard the beginning.  This a waste, Pop was not conceive for this
   obviously.  */
struct _pop_message
{
  int inbody;
  int skip_header;
  int skip_body;
  size_t body_size;
  size_t header_size;
  size_t body_lines;
  size_t header_lines;
  size_t message_size;
  size_t num;
  message_t message;
  pop_data_t mpd; /* Back pointer.  */
};

/* Structure to hold things general to the POP client, like its state, how
   many messages we have so far etc ...  */
struct _pop_data
{
  void *func;  /*  Indicate a command is in operation, busy.  */
  size_t id; /* Use in pop_expunge to hold the message num, if EAGAIN.  */
  enum pop_state state;
  pop_message_t *pmessages;
  size_t pmessages_count;
  size_t messages_count;
  size_t size;

  /* Working I/O buffers.  */
  bio_t bio;
  char *buffer;
  size_t buflen;
  char *ptr;
  char *nl;

  int is_updated;
  char *user;  /* Temporary holders for user and passwd.  */
  char *passwd;
  mailbox_t mbox; /* Back pointer.  */
} ;

/* Usefull little Macros, since these are very repetitive.  */

/* Check if we're busy ?  */
/* POP is a one channel dowload protocol, so if someone
   is trying to do another command while another is running
   something is seriously incorrect,  So the best course
   of action is to close down the connection and start a new one.
   For example mime_t only reads part of the message.  If a client
   wants to read different part of the message via mime it should
   download it first.  POP does not have the features of IMAP for
   multipart messages.  */
#define CHECK_BUSY(mbox, mpd, function, identity) \
do \
  { \
    int err = mailbox_wrlock (mbox); \
    if (err != 0) \
      return err; \
    if ((mpd->func && mpd->func != function) \
        || (mpd->id && mpd->id != (size_t)identity)) \
      { \
        mpd->id = 0; \
        mpd->func = pop_open; \
        mpd->state = POP_NO_STATE; \
        mailbox_unlock (mbox); \
        err = pop_open (mbox, mbox->flags); \
        if (err != 0) \
          { \
            return err; \
          } \
      } \
    else \
      { \
        mpd->id = (size_t)identity; \
        mpd->func = func; \
        mailbox_unlock (mbox); \
      } \
  } \
while (0)

#define CLEAR_STATE(mpd) \
 mpd->id = 0, mpd->func = NULL, mpd->state = POP_NO_STATE

/* Clear the state and close the stream.  */
#define CHECK_ERROR_CLOSE(mbox, mpd, status) \
do \
  { \
     if (status != 0) \
       { \
          stream_close (mbox->stream); \
          CLEAR_STATE (mpd); \
          return status; \
       } \
  } \
while (0)

/* Clear the state.  */
#define CHECK_ERROR(mpd, status) \
do \
  { \
     if (status != 0) \
       { \
          CLEAR_STATE (mpd); \
          return status; \
       } \
  } \
while (0)

/* Clear the state for non recoverable error.  */
#define CHECK_EAGAIN(mpd, status) \
do \
  { \
    if (status != 0) \
      { \
         if (status != EAGAIN && status != EINPROGRESS && status != EINTR) \
           { \
             CLEAR_STATE (mpd); \
           } \
         return status; \
      } \
   }  \
while (0)


/* Parse the url, allocate mailbox_t, allocate pop internal structures.  */
static int
pop_init (mailbox_t mbox)
{
  pop_data_t mpd;

  /* Allocate specifics for pop data.  */
  mpd = mbox->data = calloc (1, sizeof (*mpd));
  if (mbox->data == NULL)
    return ENOMEM;

  mpd->mbox = mbox; /* Back pointer.  */

  mpd->state = POP_NO_STATE; /* Init with no state.  */

  /* Initialize the structure.  */
  mbox->_init = pop_init;
  mbox->_destroy = pop_destroy;

  mbox->_open = pop_open;
  mbox->_close = pop_close;

  /* Messages.  */
  mbox->_get_message = pop_get_message;
  mbox->_messages_count = pop_messages_count;
  mbox->_expunge = pop_expunge;

  mbox->_scan = pop_scan;
  mbox->_is_updated = pop_is_updated;

  mbox->_size = pop_size;

  return 0; /* Okdoke.  */
}

/*  Cleaning up all the ressources associate with a pop mailbox.  */
static void
pop_destroy (mailbox_t mbox)
{
  if (mbox->data)
    {
      pop_data_t mpd = mbox->data;
      size_t i;
      mailbox_wrlock (mbox);
      /* Destroy the pop messages and ressources associated to them.  */
      for (i = 0; i < mpd->pmessages_count; i++)
	{
	  if (mpd->pmessages[i])
	    {
	      message_destroy (&(mpd->pmessages[i]->message),
			       mpd->pmessages[i]);
	      free (mpd->pmessages[i]);
	      mpd->pmessages[i] = NULL;
	    }
	}
      bio_destroy (&(mpd->bio));
      free (mpd->buffer);
      mpd->buffer = NULL;
      free (mpd->pmessages);
      mpd->pmessages = NULL;
      free (mpd);
      mbox->data = NULL;
      mailbox_unlock (mbox);
    }
}

/* User/pass authentication for pop.  */
int
pop_user (authority_t auth)
{
  mailbox_t mbox = authority_get_owner (auth);
  pop_data_t mpd = mbox->data;
  ticket_t ticket;
  int status;

  switch (mpd->state)
    {
    case POP_AUTH:
      authority_get_ticket (auth, &ticket);
      /*  Fetch the user/passwd from them.  */
      ticket_pop (ticket, "Pop User: ",  &mpd->user);
      ticket_pop (ticket, "Pop Passwd: ",  &mpd->passwd);
      status = pop_writeline (mpd, "USER %s\r\n", mpd->user);
      CHECK_ERROR_CLOSE(mbox, mpd, status);
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      free (mpd->user);
      mpd->user = NULL;
      mpd->state = POP_AUTH_USER;

    case POP_AUTH_USER:
      /* Send username.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_AUTH_USER_ACK;

    case POP_AUTH_USER_ACK:
      /* Get the user ack.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  observable_t observable = NULL;
	  mailbox_get_observable (mbox, &observable);
	  observable_notify (observable, MU_EVT_AUTHORITY_FAILED);
	  CHECK_ERROR_CLOSE (mbox, mpd, EACCES);
	}
      status = pop_writeline (mpd, "PASS %s\r\n", mpd->passwd);
      CHECK_ERROR_CLOSE (mbox, mpd, status);
      /* We have to nuke the passwd.  */
      memset (mpd->passwd, 0, strlen (mpd->passwd));
      free (mpd->passwd);
      mpd->passwd = NULL;
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, "PASS *\n");
      mpd->state = POP_AUTH_PASS;

    case POP_AUTH_PASS:
      /* Send passwd.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      /* We have to nuke the passwd.  */
      memset (mpd->buffer, 0, mpd->buflen);
      mpd->state = POP_AUTH_PASS_ACK;

    case POP_AUTH_PASS_ACK:
      /* Get the ack from passwd.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  observable_t observable = NULL;
	  mailbox_get_observable (mbox, &observable);
	  observable_notify (observable, MU_EVT_AUTHORITY_FAILED);
	  CHECK_ERROR_CLOSE (mbox, mpd, EACCES);
	}
      mpd->state = POP_AUTH_DONE;
      break;  /* We're outta here.  */

    default:
      break;
    }
  CLEAR_STATE (mpd);
  return 0;
}

/* Open the connection to the sever, ans send the authentication.
   FIXME: Should also send the CAPA command to detect for example the suport
   for POP, and DTRT(Do The Right Thing).  */
static int
pop_open (mailbox_t mbox, int flags)
{
  pop_data_t mpd = mbox->data;
  int status;
  void *func = (void *)pop_open;
  char host[256]  ;
  long port;

  /* Sanity checks.  */
  if (mbox->url == NULL || mpd == NULL)
    return EINVAL;

  mbox->flags = flags | MU_STREAM_POP;

  /* Fetch the pop server name and the port in the url_t.  */
  if ((status = url_get_host (mbox->url, host, sizeof(host), NULL)) != 0
      || (status = url_get_port (mbox->url, &port)) != 0)
    return status;

  CHECK_BUSY (mbox, mpd, func, 0);

  /* Enter the pop state machine, and boogy: AUTHORISATION State.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      /* allocate working io buffer.  */
      if (mpd->buffer == NULL)
	{
	  mpd->buflen = 255;
	  mpd->buffer = malloc (mpd->buflen + 1);
	  if (mpd->buffer == NULL)
	    {
	      CHECK_ERROR (mpd, ENOMEM);
	    }
	  mpd->ptr = mpd->buffer;
	}

      /* Create the networking stack.  */
      if (mbox->stream == NULL)
	{
	  status = tcp_stream_create (&(mbox->stream));
	  CHECK_ERROR(mpd, status);
	}
      else
	stream_close (mbox->stream);
      mpd->state = POP_OPEN_CONNECTION;

    case POP_OPEN_CONNECTION:
      /* Establish the connection.  */
      MAILBOX_DEBUG2 (mbox, MU_DEBUG_PROT, "open (%s:%d)\n", host, port);
      status = stream_open (mbox->stream, host, port, mbox->flags);
      CHECK_EAGAIN (mpd, status);
      /* Need to destroy the old bio maybe a different stream.  */
      bio_destroy (&(mpd->bio));
      /* Allocate the struct for buffered I/O.  */
      status = bio_create (&(mpd->bio), mbox->stream);
      /* Can't recover bailout.  */
      CHECK_ERROR_CLOSE (mbox, mpd, status);
      mpd->state = POP_GREETINGS;

    case POP_GREETINGS:
      {
	char auth[64] = "";
	size_t n = 0;
	/* Swallow the greetings.  */
	status = pop_read_ack (mpd);
	CHECK_EAGAIN (mpd, status);
	MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
	if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	  {
	    CHECK_ERROR_CLOSE (mbox, mpd, EACCES);
	  }

	url_get_auth (mbox->url, auth, 64, &n);
	if (n == 0 || strcasecmp (auth, "*") == 0)
	  {
	    authority_create (&(mbox->authority), mbox->ticket, mbox);
	    authority_set_authenticate (mbox->authority, pop_user, mbox);
	  }
	else if (strcasecmp (auth, "+apop") == 0)
	  {
	    /* Not supported.  */
	  }
	else
	  {
	    /* What can do flag an error ?  */
	  }
	mpd->state = POP_AUTH;
      }

    case POP_AUTH:
      status = authority_authenticate (mbox->authority);
      if (status != 0)
	return status;

    case POP_AUTH_DONE:
      break;

    default:
      /*
	fprintf (stderr, "pop_open unknown state\n");
      */
      break;
    }/* End AUTHORISATION state. */

  /* Clear any state.  */
  CLEAR_STATE (mpd);
  return 0;
}

/* Send the QUIT and close the socket.  */
static int
pop_close (mailbox_t mbox)
{
  pop_data_t mpd = mbox->data;
  void *func = (void *)pop_close;
  int status;
  size_t i;

  if (mpd == NULL)
    return EINVAL;

  /* CHECK_BUSY (mbox, mpd, func, 0); */
  mailbox_wrlock (mbox);
  if (mpd->func && mpd->func != func)
    mpd->state = POP_NO_STATE;
  mpd->id = 0;
  mpd->func = func;
  mailbox_unlock (mbox);

  /*  Ok boys, it's a wrap: UPDATE State.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      /* Initiate the close.  */
      status = pop_writeline (mpd, "QUIT\r\n");
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_QUIT;

    case POP_QUIT:
      /* Send the quit.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_QUIT_ACK;

    case POP_QUIT_ACK:
      /* Glob the acknowledge.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      /*  Now what ! and how can we tell them about errors ?  So far now
	  lets just be verbose about the error but close the connection
	  anyway.  */
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	fprintf (stderr, "pop_close: %s\n", mpd->buffer);
      stream_close (mbox->stream);
      break;

    default:
      /*
	fprintf (stderr, "pop_close unknow state");
      */
      break;
    } /* UPDATE state.  */

  /* free the messages */
  for (i = 0; i < mpd->pmessages_count; i++)
    {
      if (mpd->pmessages[i])
	{
	  message_destroy (&(mpd->pmessages[i]->message),
			   mpd->pmessages[i]);
	  free (mpd->pmessages[i]);
	  mpd->pmessages[i] = NULL;
	}
    }
  /* And clear any residue.  */
  free (mpd->pmessages);
  mpd->pmessages = NULL;
  mpd->pmessages_count = 0;
  mpd->is_updated = 0;
  bio_destroy (&(mpd->bio));
  free (mpd->buffer);
  mpd->buffer = NULL;

  CLEAR_STATE (mpd);
  return 0;
}

/*  Only build/setup the message_t structure for a mesgno.  */
static int
pop_get_message (mailbox_t mbox, size_t msgno, message_t *pmsg)
{
  pop_data_t mpd = mbox->data;
  message_t msg = NULL;
  pop_message_t mpm;
  int status;
  size_t i;

  /* Sanity.  */
  if (pmsg == NULL || mpd == NULL)
    return EINVAL;

  mailbox_rdlock (mbox);
  /* See if we have already this message.  */
  for (i = 0; i < mpd->pmessages_count; i++)
    {
      if (mpd->pmessages[i])
	{
	  if (mpd->pmessages[i]->num == msgno)
	    {
	      *pmsg = mpd->pmessages[i]->message;
	      mailbox_unlock (mbox);
	      return 0;
	    }
	}
    }
  mailbox_unlock (mbox);

  mpm = calloc (1, sizeof (*mpm));
  if (mpm == NULL)
    return ENOMEM;

  /* Back pointer.  */
  mpm->mpd = mpd;
  mpm->num = msgno;

  /* Create the message.  */
  {
    stream_t stream = NULL;
    if ((status = message_create (&msg, mpm)) != 0
	|| (status = stream_create (&stream, mbox->flags, msg)) != 0)
      {
	stream_destroy (&stream, msg);
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    stream_set_read (stream, pop_message_read, msg);
    stream_set_fd (stream, pop_message_fd, msg);
    message_set_stream (msg, stream, mpm);
    message_set_size (msg, pop_message_size, mpm);
  }

  /* Create the header.  */
  {
    header_t header = NULL;
    stream_t stream = NULL;
    if ((status = header_create (&header, NULL, 0,  msg)) != 0
	|| (status = stream_create (&stream, mbox->flags, header)) != 0)
      {
	stream_destroy (&stream, header);
	header_destroy (&header, msg);
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    stream_set_read (stream, pop_top, header);
    stream_set_fd (stream, pop_header_fd, header);
    header_set_stream (header, stream, msg);
    message_set_header (msg, header, mpm);
  }

  /* Create the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, msg);
    if (status != 0)
      {
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    attribute_set_get_flags (attribute, pop_attr_flags, msg);
    message_set_attribute (msg, attribute, mpm);
  }

  /* Create the body and its stream.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, msg)) != 0
	|| (status = stream_create (&stream, mbox->flags, body)) != 0)
      {
	body_destroy (&body, msg);
	stream_destroy (&stream, body);
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    stream_set_read (stream, pop_body_read, body);
    stream_set_fd (stream, pop_body_fd, body);
    body_set_size (body, pop_body_size, msg);
    body_set_lines (body, pop_body_lines, msg);
    body_set_stream (body, stream, msg);
    message_set_body (msg, body, mpm);
  }

  /* Set the UIDL call on the message. */
  message_set_uidl (msg, pop_uidl, mpm);

  /* Add it to the list.  */
  mailbox_wrlock (mbox);
  {
    pop_message_t *m ;
    m = realloc (mpd->pmessages, (mpd->pmessages_count + 1)*sizeof (*m));
    if (m == NULL)
      {
	message_destroy (&msg, mpm);
	free (mpm);
	mailbox_unlock (mbox);
	return ENOMEM;
      }
    mpd->pmessages = m;
    mpd->pmessages[mpd->pmessages_count] = mpm;
    mpd->pmessages_count++;
  }
  mailbox_unlock (mbox);

  /* Save The message.  */
  *pmsg = mpm->message = msg;

  return 0;
}

/*  How many messages we have.  Done with STAT.  */
static int
pop_messages_count (mailbox_t mbox, size_t *pcount)
{
  pop_data_t mpd = mbox->data;
  int status;
  void *func = (void *)pop_messages_count;

  if (mpd == NULL)
    return EINVAL;

  /* Do not send a STAT if we know the answer.  */
  if (pop_is_updated (mbox))
    {
      if (pcount)
	*pcount = mpd->messages_count;
      return 0;
    }

  /* Flag busy.  */
  CHECK_BUSY (mbox, mpd, func, 0);

  /* TRANSACTION state.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      status = pop_writeline (mpd, "STAT\r\n");
      CHECK_ERROR (mpd, status);
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_STAT;

    case POP_STAT:
      /* Send the STAT.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_STAT_ACK;

    case POP_STAT_ACK:
      /* Get the ACK.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
      break;

    default:
      /*
	fprintf (stderr, "pomp_messages_count: unknow state\n");
      */
      break;
    }

  /* Parse the answer.  */
  status = sscanf (mpd->buffer, "+OK %d %d", &(mpd->messages_count),
		   &(mpd->size));

  /*  Clear the state _after_ the scanf, since another thread could
      start writing over mpd->buffer.  But We're not thread safe yet.  */
  CLEAR_STATE (mpd);

  if (status == EOF || status != 2)
    return EIO;

  if (pcount)
    *pcount = mpd->messages_count;
  mpd->is_updated = 1;
  return 0;
}


/* Update and scanning.  */
static int
pop_is_updated (mailbox_t mbox)
{
  pop_data_t mpd = mbox->data;
  if (mpd == NULL)
    return 0;
  return mpd->is_updated;
}

/* We just simulated. By sending a notification for the total msgno.  */
/* FIXME is message is set deleted should we sent a notif ?  */
static int
pop_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  int status;
  size_t i;

  status = pop_messages_count (mbox, pcount);
  if (status != 0)
    return status;
  if (mbox->observable == NULL)
    return 0;
  for (i = msgno; i <= *pcount; i++)
    if (observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD) != 0)
      break;
  return 0;
}

/* This were we actually send the DELE command.  */
static int
pop_expunge (mailbox_t mbox)
{
  pop_data_t mpd = mbox->data;
  size_t i;
  attribute_t attr;
  int status;
  void *func = (void *)pop_expunge;

  if (mpd == NULL)
    return EINVAL;

  /* Busy ?  */
  CHECK_BUSY (mbox, mpd, func, 0);

  for (i = (int)mpd->id; i < mpd->pmessages_count; mpd->id = ++i)
    {
      if (message_get_attribute (mpd->pmessages[i]->message, &attr) == 0)
	{
	  if (attribute_is_deleted (attr))
	    {
	      switch (mpd->state)
		{
		case POP_NO_STATE:
		  status = pop_writeline (mpd, "DELE %d\r\n",
					  mpd->pmessages[i]->num);
		  CHECK_ERROR (mpd, status);
		  MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
		  mpd->state = POP_DELE;

		case POP_DELE:
		  /* Send DELETE.  */
		  status = pop_write (mpd);
		  CHECK_EAGAIN (mpd, status);
		  mpd->state = POP_DELE_ACK;

		case POP_DELE_ACK:
		  status = pop_read_ack (mpd);
		  CHECK_EAGAIN (mpd, status);
		  MAILBOX_DEBUG0 (mbox, MU_DEBUG_PROT, mpd->buffer);
		  if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
		    {
		      CHECK_ERROR (mpd, ERANGE);
		    }
		  mpd->state = POP_NO_STATE;
		  break;

		default:
		  /* fprintf (stderr, "pop_expunge: unknow state\n"); */
		  break;
		} /* switch (state) */
	    } /* if attribute_is_deleted() */
	} /* message_get_attribute() */
    } /* for */
  CLEAR_STATE (mpd);
  /* Invalidate.  But Really they should shutdown the channel POP protocol
     is not meant for this like IMAP.  */
  mpd->is_updated = 0;
  return 0;
}

/* Mailbox size ? It is part of the STAT command */
static int
pop_size (mailbox_t mbox, off_t *psize)
{
  pop_data_t mpd = mbox->data;
  int status = 0;

  if (mpd == NULL)
    return EINVAL;

  if (! pop_is_updated (mbox))
    status = pop_messages_count (mbox, &mpd->size);
  if (psize)
    *psize = mpd->size;
  return status;
}

/* Form the RFC:
   "It is important to note that the octet count for a message on the
   server host may differ from the octet count assigned to that message
   due to local conventions for designating end-of-line.  Usually,
   during the AUTHORIZATION state of the POP3 session, the POP3 server
   can calculate the size of each message in octets when it opens the
   maildrop.  For example, if the POP3 server host internally represents
   end-of-line as a single character, then the POP3 server simply counts
   each occurrence of this character in a message as two octets."

   This is not perfect if we do not know the number of lines in the message
   then the octets returned will not be correct so we do our best.
 */
static int
pop_message_size (message_t msg, size_t *psize)
{
  pop_message_t mpm = message_get_owner (msg);
  pop_data_t mpd;
  int status = 0;
  void *func = (void *)pop_message_size;
  size_t num;

  if (mpm == NULL)
    return EINVAL;

  /* Did we have it already ?  */
  if (mpm->message_size != 0)
    {
      *psize = mpm->message_size;
      return 0;
    }

  mpd = mpm->mpd;
  /* Busy ? */
  CHECK_BUSY (mpd->mbox, mpd, func, msg);

  /* Get the size.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      status = pop_writeline (mpd, "LIST %d\r\n", mpm->num);
      CHECK_ERROR (mpd, status);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_LIST;

    case POP_LIST:
      /* Send the LIST.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_LIST_ACK;

    case POP_LIST_ACK:
      /* Resp from LIST. */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      break;

    default:
      /*
	fprintf (stderr, "pop_uidl state\n");
      */
      break;
    }

  status = sscanf (mpd->buffer, "+OK %d %d\n", &num, &mpm->message_size);
  CLEAR_STATE (mpd);

  if (status != 2)
    status = EINVAL;

  if (psize)
    *psize = mpm->message_size;
  return 0;
}

static int
pop_body_size (body_t body, size_t *psize)
{
  message_t msg = body_get_owner (body);
  pop_message_t mpm = message_get_owner (msg);

  if (mpm == NULL)
    return EINVAL;

  /* Did we have it already ?  */
  if (mpm->body_size != 0)
    {
      *psize = mpm->body_size;
    }
  else if (mpm->message_size != 0)
    {
      *psize = mpm->message_size - mpm->header_size - mpm->body_lines;
    }
  else
    *psize = 0;

  return 0;
}

static int
pop_body_lines (body_t body, size_t *plines)
{
  message_t msg = body_get_owner (body);
  pop_message_t mpm = message_get_owner (msg);
  if (mpm == NULL)
    return EINVAL;
  if (plines)
    *plines = mpm->body_lines;
  return 0;
}

/* Pop does not have any command for this, We fake by reading the "Status: "
   header.  But this is hackish some POP server(Qpopper) skip it.  Also
   because we call header_get_value the function may return EAGAIN...
   uncool.  */
static int
pop_attr_flags (attribute_t attr, int *pflags)
{
  message_t msg = attribute_get_owner (attr);
  pop_message_t mpm = message_get_owner (msg);
  char hdr_status[64];
  header_t header = NULL;
  int err;

  if (mpm == NULL)
    return EINVAL;
  hdr_status[0] = '\0';
  message_get_header (mpm->message, &header);
  err  = header_get_value (header, "Status",
			   hdr_status, sizeof (hdr_status), NULL);
  if (err != 0)
    err = string_to_flags (hdr_status, pflags);
  return err;
}

/* stub to call from body object.  */
static int
pop_body_fd (stream_t stream, int *pfd)
{
  body_t body = stream_get_owner (stream);
  message_t msg = body_get_owner (body);
  pop_message_t mpm = message_get_owner (msg);
  return pop_get_fd (mpm, pfd);
}

/* stub to call from header object.  */
static int
pop_header_fd (stream_t stream, int *pfd)
{
  header_t header = stream_get_owner (stream);
  message_t msg = header_get_owner (header);
  pop_message_t mpm = message_get_owner (msg);
  return pop_get_fd (mpm, pfd);
}

/* stub to call from message object.  */
static int
pop_message_fd (stream_t stream, int *pfd)
{
  message_t msg = stream_get_owner (stream);
  pop_message_t mpm = message_get_owner (msg);
  return pop_get_fd (mpm, pfd);
}

static int
pop_get_fd (pop_message_t mpm, int *pfd)
{
  if (mpm && mpm->mpd && mpm->mpd->mbox)
	return stream_get_fd (mpm->mpd->mbox->stream, pfd);
  return EINVAL;
}

/* Get the UIDL.  Client should be prepare since it may fail.  UIDL is
   optionnal for many POP server.
   FIXME:  We should check this with CAPA and fall back to md5 scheme ?
   Or maybe check for "X-UIDL" a la Qpopper ?  */
static int
pop_uidl (message_t msg, char *buffer, size_t buflen, size_t *pnwriten)
{
  pop_message_t mpm = message_get_owner (msg);
  pop_data_t mpd;
  int status = 0;
  void *func = (void *)pop_uidl;
  size_t num;
  /* According to the RFC uidl's are no longer then 70 chars.  */
  char uniq[128];

  if (mpm == NULL)
    return EINVAL;

  mpd = mpm->mpd;

  /* Busy ? */
  CHECK_BUSY (mpd->mbox, mpd, func, 0);

  /* Get the UIDL.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      status = pop_writeline (mpd, "UIDL %d\r\n", mpm->num);
      CHECK_ERROR (mpd, status);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_UIDL;

    case POP_UIDL:
      /* Send the UIDL.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_UIDL_ACK;

    case POP_UIDL_ACK:
      /* Resp from UIDL. */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      break;

    default:
      /*
	fprintf (stderr, "pop_uidl state\n");
      */
      break;
    }

  CLEAR_STATE (mpd);

  /* FIXME:  I should cache the result.  */
  *uniq = '\0';
  status = sscanf (mpd->buffer, "+OK %d %127s\n", &num, uniq);
  if (status != 2)
    {
      status = EINVAL;
      buflen = 0;
    }
  else
    {
      num = strlen (uniq);
      uniq[num - 1] = '\0'; /* Nuke newline.  */
      buflen = (buflen < num) ? buflen : num;
      memcpy (buffer, uniq, buflen);
      buffer [buflen - 1] = '\0';
      status = 0;
    }
  if (pnwriten)
    *pnwriten = buflen;
  return status;
}

/* How we retrieve the headers.  If it fails we jump to the pop_retr()
   code .i.e send a RETR and skip the body, ugly.
   NOTE:  offset is meaningless.  */
static int
pop_top (stream_t is, char *buffer, size_t buflen,
	 off_t offset, size_t *pnread)
{
  header_t header = stream_get_owner (is);
  message_t msg = header_get_owner (header);
  pop_message_t mpm = message_get_owner (msg);
  pop_data_t mpd;
  size_t nread = 0;
  int status = 0;
  void *func = (void *)pop_top;

  if (mpm == NULL)
    return EINVAL;

  mpd = mpm->mpd;

  /* We do not carry the offset backward(for pop), should be doc somewhere.  */
  if ((size_t)offset < mpm->header_size)
    return ESPIPE;

  /* Busy ? */
  CHECK_BUSY (mpd->mbox, mpd, func, msg);

  /* Get the header.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      /* TOP is an optionnal command, if we want to be compliant we can not
	 count on it to exists. So we should be prepare when it fails and
	 fall to a second scheme.  */
      status = pop_writeline (mpd, "TOP %d 0\r\n", mpm->num);
      CHECK_ERROR (mpd, status);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_TOP;

    case POP_TOP:
      /* Send the TOP.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_TOP_ACK;

    case POP_TOP_ACK:
      /* Ack from TOP. */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  /* fprintf (stderr, "TOP not implemented\n"); */
	  /* Fall back to RETR call.  */
	  mpd->state = POP_NO_STATE;
	  mpm->skip_header = 0;
	  mpm->skip_body = 1;
	  return pop_retr (mpm, buffer, buflen, offset, pnread);
	}
      mpd->state = POP_TOP_RX;

    case POP_TOP_RX:
      /* Get the header.  */
      do
	{
	  /* Seek in position.  */
	  ssize_t pos = offset - mpm->header_size;
	  /* Do we need to fill up.  */
	  if (mpd->nl == NULL || mpd->ptr == mpd->buffer)
	    {
	      status = pop_readline (mpd);
	      CHECK_EAGAIN (mpd, status);
	      mpm->header_lines++;
	    }
	  /* If we have to skip some data to get to the offet.  */
	  if (pos > 0)
	    nread = fill_buffer (mpd, NULL, pos);
	  else
	    nread = fill_buffer (mpd, buffer, buflen);
	  mpm->header_size += nread;
	}
      while (nread > 0 && (size_t)offset > mpm->header_size);
      break;

    default:
      /* Probaly TOP was not supported so we have fall back to RETR.  */
      mpm->skip_header = 0;
      mpm->skip_body = 1;
      return pop_retr (mpm, buffer, buflen, offset, pnread);
    } /* switch (state) */

  if (nread == 0)
    {
      CLEAR_STATE (mpd);
    }
  if (pnread)
    *pnread = nread;
  return 0;
}

/* Stub to call pop_retr ().  */
static int
pop_header_read (stream_t is, char *buffer, size_t buflen, off_t offset,
		 size_t *pnread)
{
  message_t msg = stream_get_owner (is);
  pop_message_t mpm = message_get_owner (msg);
  pop_data_t mpd;
  void *func = (void *)pop_header_read;

  if (mpm == NULL)
    return EINVAL;
  mpd = mpm->mpd;

  /* Busy ? */
  CHECK_BUSY (mpd->mbox, mpd, func, msg);

  mpm->skip_header = 0;
  mpm->skip_body = 1;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

/* Stub to call pop_retr (). Call from the stream object of the body.  */
static int
pop_body_read (stream_t is, char *buffer, size_t buflen, off_t offset,
	       size_t *pnread)
{
  body_t body = stream_get_owner (is);
  message_t msg = body_get_owner (body);
  pop_message_t mpm = message_get_owner (msg);
  pop_data_t mpd;
  void *func = (void *)pop_body_read;

  if (mpm == NULL)
    return EINVAL;
  mpd = mpm->mpd;

  /* Busy ? */
  CHECK_BUSY (mpd->mbox, mpd, func, msg);

  mpm->skip_header = 1;
  mpm->skip_body = 0;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

/* Stub to call pop_retr (), calling from the stream object of a message.  */
static int
pop_message_read (stream_t is, char *buffer, size_t buflen, off_t offset,
		  size_t *pnread)
{
  message_t msg = stream_get_owner (is);
  pop_message_t mpm = message_get_owner (msg);
  pop_data_t mpd;
  void *func = (void *)pop_message_read;

  if (mpm == NULL)
    return EINVAL;
  mpd = mpm->mpd;

  /* Busy ? */
  CHECK_BUSY (mpd->mbox, mpd, func, msg);

  mpm->skip_header = mpm->skip_body = 0;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

/* Little helper to fill the buffer without overflow.  */
static int
fill_buffer (pop_data_t mpd, char *buffer, size_t buflen)
{
  int nleft, n, nread = 0;

  /* How much we can copy ?  */
  n = mpd->ptr - mpd->buffer;
  nleft = buflen - n;

 /* We got more then requested.  */
  if (nleft < 0)
    {
      size_t sentinel;
      nread = buflen;
      sentinel = mpd->ptr - (mpd->buffer + nread);
      if (buffer)
	memcpy (buffer, mpd->buffer, nread);
      memmove (mpd->buffer, mpd->buffer + nread, sentinel);
      mpd->ptr = mpd->buffer + sentinel;
    }
  else
    {
      /* Drain our buffer.  */;
      nread = n;
      if (buffer)
	memcpy (buffer, mpd->buffer, nread);
      mpd->ptr = mpd->buffer;
    }

  return nread;
}

/* The heart of most funtions.  Send the RETR and skip different parts.  */
static int
pop_retr (pop_message_t mpm, char *buffer, size_t buflen, off_t offset,
	  size_t *pnread)
{
  pop_data_t mpd;
  size_t nread = 0;
  int status = 0;
  size_t oldbuflen = buflen;

  (void)offset;

  mpd = mpm->mpd;

  if (pnread)
    *pnread = nread;

  /*  Take care of the obvious.  */
  if (buffer == NULL || buflen == 0)
    {
      CLEAR_STATE (mpd);
      return 0;
    }

  /* We do not carry the offset backward(for pop), should be doc somewhere.  */
  /*if (offset < mpm->header_size)
    return ESPIPE;
  */

  /* pop_retr() is not call directly so we assume that the locks were set.  */

  switch (mpd->state)
    {
    case POP_NO_STATE:
      mpm->body_lines = mpm->body_size = 0;
      status = pop_writeline (mpd, "RETR %d\r\n", mpm->num);
      MAILBOX_DEBUG0 (mpd->mbox, MU_DEBUG_PROT, mpd->buffer);
      CHECK_ERROR (mpd, status);
      mpd->state = POP_RETR;

    case POP_RETR:
      /* Send the RETR command.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_RETR_ACK;

    case POP_RETR_ACK:
      /* RETR ACK.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  CHECK_ERROR (mpd, EACCES);
	}
      mpd->state = POP_RETR_RX_HDR;

    case POP_RETR_RX_HDR:
      /* Skip/Take the header.  */
      while (!mpm->inbody)
        {
	  /* Do we need to fill up.  */
	  if (mpd->nl == NULL || mpd->ptr == mpd->buffer)

	    {
	      status = pop_readline (mpd);
	      if (status != 0)
		{
		  /* Do we have something in the buffer flush it first.  */
		  if (buflen != oldbuflen)
		    return 0;
		  CHECK_EAGAIN (mpd, status);
		}
	      mpm->header_lines++;
	    }
	  /* Oops !! Hello houston we have a major problem here.  */
	  if (mpd->buffer[0] == '\0')
	    {
	      /* Still Do the right thing.  */
	      if (buflen != oldbuflen)
		{
		  CLEAR_STATE (mpd);
		}
	      else
		mpd->state = POP_STATE_DONE;
	      return 0;
	    }
	  /* The problem is that we are using RETR instead of TOP to retreive
	     headers, i.e the server contacted does not support it.  So we
	     have to make sure that the next line really means end of the
	     headers.  Still some cases we may loose.  But 99.9% of POPD
	     encounter support TOP.  In the 0.1% case get GNU pop3d, or the
	     hack below will suffice.  */
	  if (mpd->buffer[0] == '\n' && mpd->buffer[1] == '\0')
	    mpm->inbody = 1; /* break out of the while.  */
	  if (!mpm->skip_header)
	    {
	      ssize_t pos = offset  - mpm->header_size;
	      if (pos)
		{
		  nread = fill_buffer (mpd, NULL, pos);
		  mpm->header_size += nread;
		}
	      else
		{
		  nread = fill_buffer (mpd, buffer, buflen);
		  mpm->header_size += nread;
		  if (pnread)
		    *pnread += nread;
		  buflen -= nread ;
		  if (buflen > 0)
		    buffer += nread;
		  else
		    return 0;
		}
	    }
	  else
	    mpd->ptr = mpd->buffer;
	}
      mpd->state = POP_RETR_RX_BODY;

    case POP_RETR_RX_BODY:
      /* Start/Take the body.  */
      while (mpm->inbody)
	{
	  /* Do we need to fill up.  */
	  if (mpd->nl == NULL || mpd->ptr == mpd->buffer)
	    {
	      status = pop_readline (mpd);
	      if (status != 0)
		{
		  /* Flush The Buffer ?  */
		  if (buflen != oldbuflen)
		    return 0;
		  CHECK_EAGAIN (mpd, status);
		}
	      mpm->body_lines++;
	    }

	    if (mpd->buffer[0] == '\0')
	      mpm->inbody = 0; /* Breakout of the while.  */

	    if (!mpm->skip_body)
	      {
		ssize_t pos = offset - mpm->body_size;
		if (pos > 0)
		  {
		    nread = fill_buffer (mpd, NULL, pos);
		    mpm->body_size += nread;
		  }
		else
		  {
		    nread = fill_buffer (mpd, buffer, buflen);
		    mpm->body_size += nread;
		    if (pnread)
		      *pnread += nread;
		    buflen -= nread ;
		    if (buflen > 0)
		      buffer += nread;
		    else
		      return 0;
		  }
	      }
	    else
	      {
		mpm->body_size += (mpd->ptr - mpd->buffer);
		mpd->ptr = mpd->buffer;
	      }
	  }
      mpm->message_size = mpm->body_size + mpm->header_size;
      mpd->state = POP_STATE_DONE;
      /* Return here earlier, because we want to return nread = 0 to notify
	 the callee that we've finish, since there is already data in the
	 we have to return them first and _then_ tell them its finish.  If we
	 don't we will start over again by sending another RETR.  */
      if (buflen != oldbuflen)
	return 0;

    case POP_STATE_DONE:
      /* A convenient break, this is here so we can return 0 read on next
	 call meaning we're done.  */

    default:
      /* fprintf (stderr, "pop_retr unknow state\n"); */
      break;
    } /* Switch state.  */

  CLEAR_STATE (mpd);
  mpm->skip_header = mpm->skip_body = 0;
  return 0;
}

static int
pop_writeline (pop_data_t mpd, const char *format, ...)
{
  int len;
  va_list ap;

  va_start(ap, format);
  do
    {
      len = vsnprintf (mpd->buffer, mpd->buflen - 1, format, ap);
      if (len >= (int)mpd->buflen)
	{
	  mpd->buflen *= 2;
	  mpd->buffer = realloc (mpd->buffer, mpd->buflen);
	  if (mpd->buffer == NULL)
	    return ENOMEM;
	}
    }
  while (len > (int)mpd->buflen);
  va_end(ap);
  mpd->ptr = mpd->buffer + len;
  return 0;
}

static int
pop_write (pop_data_t mpd)
{
  int status = 0;
  size_t len;
  if (mpd->ptr > mpd->buffer)
    {
      len = mpd->ptr - mpd->buffer;
      status = bio_write (mpd->bio, mpd->buffer, len, &len);
      if (status == 0)
	{
	  memmove (mpd->buffer, mpd->buffer + len, len);
	  mpd->ptr -= len;
	}
    }
  else
    {
      mpd->ptr = mpd->buffer;
      len = 0;
    }
  return status;
}

static int
pop_read_ack (pop_data_t mpd)
{
  int status = pop_readline (mpd);
  if (status == 0)
    mpd->ptr = mpd->buffer;
  return status;
}

/* Read a complete line form the pop server. Transform CRLF to LF, remove
   the stuff by termination octet ".", put a null in the buffer when done.  */
static int
pop_readline (pop_data_t mpd)
{
  size_t n = 0;
  size_t total = mpd->ptr - mpd->buffer;
  int status;

  /* Must get a full line before bailing out.  */
  do
    {
      status = bio_readline (mpd->bio, mpd->buffer + total,
			     mpd->buflen - total, &n);
      if (status != 0)
	return status;

      total += n;
      mpd->nl = memchr (mpd->buffer, '\n', total);
      if (mpd->nl == NULL)  /* Do we have a full line.  */
	{
	  /* Allocate a bigger buffer ?  */
	  if (total >= mpd->buflen -1)
	    {
	      mpd->buflen *= 2;
	      mpd->buffer = realloc (mpd->buffer, mpd->buflen + 1);
	      if (mpd->buffer == NULL)
		return ENOMEM;
	    }
	}
      mpd->ptr = mpd->buffer + total;
    }
  while (mpd->nl == NULL);

  /* When examining a multi-line response, the client checks to see if the
     line begins with the termination octet "."(DOT). If yes and if octets
     other than CRLF follow, the first octet of the line (the termination
     octet) is stripped away.  */
  if (total >= 3  && mpd->buffer[0] == '.')
    {
      if (mpd->buffer[1] != '\r' && mpd->buffer[2] != '\n')
	{
	  memmove (mpd->buffer, mpd->buffer + 1, total - 1);
	  mpd->ptr--;
	  mpd->nl--;
	}
      /* And if CRLF immediately follows the termination character, then the
	 response from the POP server is ended and the line containing
	 ".CRLF" is not considered part of the multi-line response.  */
      else if (mpd->buffer[1] == '\r' && mpd->buffer[2] == '\n')
	{
	  mpd->buffer[0] = '\0';
	  mpd->ptr = mpd->buffer;
	  mpd->nl = NULL;
	}
    }
  /* \r\n --> \n\0  */
  if (mpd->nl > mpd->buffer)
    {
      *(mpd->nl - 1) = '\n';
      *(mpd->nl) = '\0';
      mpd->ptr = mpd->nl;
    }
  return 0;
}
