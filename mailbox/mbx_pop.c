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

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>

#include <mailbox0.h>
#include <stream0.h>
#include <body0.h>
#include <message0.h>
#include <registrar0.h>
#include <auth0.h>
#include <header0.h>
#include <attribute0.h>
#include <bio.h>
#include <ctype.h>

/* Advance declarations.  */
struct _pop_data;
struct _pop_message;

typedef struct _pop_data * pop_data_t;
typedef struct _pop_message * pop_message_t;

/* The different possible states of a Pop client, Note that POP3 is not
   reentrant. It is only one channel.  */
enum pop_state
{
  POP_NO_STATE = 0, POP_STATE_DONE,
  POP_OPEN_CONNECTION,
  POP_GREETINGS,
  POP_APOP_TX, POP_APOP_ACK,
  POP_DELE_TX, POP_DELE_ACK,
  POP_LIST_TX, POP_LIST_ACK, POP_LIST_RX,
  POP_PASS_TX, POP_PASS_ACK,
  POP_QUIT_TX, POP_QUIT_ACK,
  POP_NOOP_TX, POP_NOOP_ACK,
  POP_RETR_TX, POP_RETR_ACK, POP_RETR_RX_HDR, POP_RETR_RX_BODY,
  POP_RSET_TX, POP_RSET_ACK,
  POP_STAT_TX, POP_STAT_ACK,
  POP_TOP_TX,  POP_TOP_ACK,  POP_TOP_RX,
  POP_UIDL_TX, POP_UIDL_ACK,
  POP_USER_TX, POP_USER_ACK,
};

/*  Those two are exportable funtions i.e. they are visible/call when you
    call a URL "pop://..." to mailbox_create.  */

static int pop_create (mailbox_t *, const char *);
static void pop_destroy (mailbox_t *);

struct mailbox_registrar _mailbox_pop_registrar =
{
  "POP3",
  pop_create, pop_destroy
};

/*  Functions/Methods that implements the mailbox_t API.  */
static int pop_open (mailbox_t, int);
static int pop_close (mailbox_t);
static int pop_get_message (mailbox_t, size_t, message_t *);
static int pop_messages_count (mailbox_t, size_t *);
static int pop_expunge (mailbox_t);
static int pop_num_deleted (mailbox_t, size_t *);
static int pop_scan (mailbox_t, size_t, size_t *);
static int pop_is_updated (mailbox_t);

/* The implementation of message_t */
static int pop_size (mailbox_t, off_t *);
static int pop_top (stream_t, char *, size_t, off_t, size_t *);
static int pop_read_header (stream_t, char *, size_t, off_t, size_t *);
static int pop_read_body (stream_t, char *, size_t, off_t, size_t *);
static int pop_read_message (stream_t, char *, size_t, off_t, size_t *);
static int pop_retr (pop_message_t, char *, size_t, off_t, size_t *);
static int pop_get_fd (stream_t, int *);
static int pop_get_flags (attribute_t, int *);
static int pop_body_size (body_t, size_t *);
static int pop_body_lines (body_t body, size_t *plines);
static int pop_uidl (message_t, char *, size_t, size_t *);
static int fill_buffer (pop_data_t mpd, char *buffer, size_t buflen);
static int pop_readline (pop_data_t mpd);
static int pop_read_ack (pop_data_t mpd);
static int pop_writeline (pop_data_t mpd, const char *format, ...);
static int pop_write (pop_data_t mpd);

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
  size_t body_lines;
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
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  int flags;  /* Flags of for the stream_t object.  */

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

/* Usefull little Macros, since this is very repetitive.  */
#define CLEAR_STATE(mpd) \
 mpd->id = 0, mpd->func = NULL, \
 mpd->state = POP_NO_STATE

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
pop_create (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  pop_data_t mpd;
  size_t name_len;

  /* Sanity check.  */
  if (name == NULL || *name == '\0')
    return EINVAL;

  name_len = strlen (name);

#define POP_SCHEME "pop://"
#define POP_SCHEME_LEN 6
#define SEPARATOR '/'

  /* Skip the url scheme.  */
  if (name_len > POP_SCHEME_LEN &&
      (name[0] == 'p' || name[0] == 'P') &&
      (name[1] == 'o' || name[1] == 'O') &&
      (name[2] == 'p' || name[2] == 'P') &&
      (name[3] == ':' && name[4] == '/' && name[5] == '/'))
    {
      name += POP_SCHEME_LEN;
      name_len -= POP_SCHEME_LEN;
    }

  /* Allocate memory for mbox.  */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    return ENOMEM;

  /* Allocate specifics for pop data.  */
  mpd = mbox->data = calloc (1, sizeof (*mpd));
  if (mbox->data == NULL)
    {
      pop_destroy (&mbox);
      return ENOMEM;
    }
  mpd->state = POP_NO_STATE;
  mpd->mbox = mbox;

  /* Copy the name.  */
  mbox->name = calloc (name_len + 1, sizeof (char));
  if (mbox->name == NULL)
    {
      pop_destroy (&mbox);
      return ENOMEM;
    }
  memcpy (mbox->name, name, name_len);

#ifdef HAVE_PHTREAD_H
  /* Mutex when accessing the structure fields.  */
  /* FIXME: should we use rdwr locks instead ??  */
  /* XXXX:  This is not use yet and the library is still not thread safe. This
     is more a gentle remider that I got more work to do.  */
  pthread_mutex_init (&(mpd->mutex), NULL);
#endif

  /* Initialize the structure.  */
  mbox->_create = pop_create;
  mbox->_destroy = pop_destroy;

  mbox->_open = pop_open;
  mbox->_close = pop_close;

  /* Messages.  */
  mbox->_get_message = pop_get_message;
  mbox->_messages_count = pop_messages_count;
  mbox->_expunge = pop_expunge;
  mbox->_num_deleted = pop_num_deleted;

  mbox->_scan = pop_scan;
  mbox->_is_updated = pop_is_updated;

  mbox->_size = pop_size;

  (*pmbox) = mbox;

  return 0; /* Okdoke.  */
}

/*  Cleaning up all the ressources associate with a pop mailbox.  */
static void
pop_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      mailbox_t mbox = *pmbox;
      if (mbox->data)
	{
	  pop_data_t mpd = mbox->data;
	  size_t i;
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
#ifdef HAVE_PHTREAD_H
	  pthread_mutex_destroy (&(mpd->mutex));
#endif
	  free (mpd);
	  mbox->data = NULL;
	}
      /* Since the mailbox is destroy, close the stream but not via
	 pop_close () which can return EAGAIN, or other unpleasant side
	 effects, but rather the hard way.  */
      stream_close (mbox->stream);
      stream_destroy (&(mbox->stream), mbox);
      auth_destroy (&(mbox->auth), mbox);
      if (mbox->name)
	{
	  free (mbox->name);
	  mbox->name = NULL;
	}
      if (mbox->event)
	{
	  free (mbox->event);
	  mbox->event = NULL;
	}
      if (mbox->url)
	url_destroy (&(mbox->url));
      free (mbox);
      *pmbox = NULL;
    }
}

/*  We should probably use getpass () or something similar.  */
static struct termios stored_settings;

static void
echo_off(void)
{
  struct termios new_settings;
  tcgetattr (0, &stored_settings);
  new_settings = stored_settings;
  new_settings.c_lflag &= (~ECHO);
  tcsetattr (0, TCSANOW, &new_settings);
}

static void
echo_on(void)
{
  tcsetattr (0, TCSANOW, &stored_settings);
}

/* User/pass authentication for pop.  */
static int
pop_authenticate (auth_t auth, char **user, char **passwd)
{
  /* FIXME: This incorrect and goes against GNU style: having a limitation on
     the user/passwd length, it should be fix.  */
  char u[128];
  char p[128];
  mailbox_t mbox = auth->owner;
  int status;

  *u = '\0';
  *p = '\0';

  /* Prompt for the user/login name.  */
  status = url_get_user (mbox->url, u, sizeof (u), NULL);
  if (status != 0 || *u == '\0')
    {
      printf ("Pop User: "); fflush (stdout);
      fgets (u, sizeof (u), stdin);
      u [strlen (u) - 1] = '\0'; /* nuke the trailing NL */
    }
  /* Prompt for the passwd. */
  status = url_get_passwd (mbox->url, p, sizeof (p), NULL);
  if (status != 0 || *p == '\0' || *p == '*')
    {
      printf ("Pop Passwd: ");
      fflush (stdout);
      echo_off ();
      fgets (p, sizeof(p), stdin);
      echo_on ();
      p [strlen (p) - 1] = '\0';
    }
  *user = strdup (u);
  *passwd = strdup (p);
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

  /* Fetch the pop server name and the port in the url_t.  */
  if ((status = url_get_host (mbox->url, host, sizeof(host), NULL)) != 0
      || (status = url_get_port (mbox->url, &port)) != 0)
    return status;

  /* Dealing whith Authentication.  So far only normal user/pass supported.  */
  if (mbox->auth == NULL)
    {
      status = auth_create (&(mbox->auth), mbox);
      if (status != 0)
	return status;
      auth_set_authenticate (mbox->auth, pop_authenticate, mbox);
    }

  /* Flag busy.  */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;

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

      /* Spawn auth prologue.  */
      auth_prologue (mbox->auth);

      /* Create the networking stack.  */
      if (mbox->stream == NULL)
	{
	  status = tcp_stream_create (&(mbox->stream));
	  CHECK_ERROR(mpd, status);
	}
      mpd->state = POP_OPEN_CONNECTION;

    case POP_OPEN_CONNECTION:
      /* Establish the connection.  */
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, "open (%s:%d)\n",
		     host, port);
      status = stream_open (mbox->stream, host, port, flags);
      if (status != 0)
	{
	  /* Clear the state for non recoverable error.  */
	  if (status != EAGAIN && status != EINPROGRESS && status != EINTR)
	    {
	      CLEAR_STATE (mpd);
	    }
	  return status;
	}
      /* Need to destroy the old bio maybe a different stream.  */
      bio_destroy (&(mpd->bio));
      /* Allocate the struct for buffered I/O.  */
      status = bio_create (&(mpd->bio), mbox->stream);
      /* Can't recover bailout.  */
      CHECK_ERROR_CLOSE (mbox, mpd, status);
      mpd->state = POP_GREETINGS;

    case POP_GREETINGS:
      /* Swallow the greetings.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  CHECK_ERROR_CLOSE (mbox, mpd, EACCES);
	}

      /*  Fetch the user/passwd from them.  */
      auth_authenticate (mbox->auth, &mpd->user, &mpd->passwd);

      status = pop_writeline (mpd, "USER %s\r\n", mpd->user);
      CHECK_ERROR_CLOSE(mbox, mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      free (mpd->user);
      mpd->user = NULL;
      mpd->state = POP_USER_TX;

    case POP_USER_TX:
      /* Send username.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_USER_ACK;

    case POP_USER_ACK:
      /* Get the user ack.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  CHECK_ERROR_CLOSE (mbox, mpd, EACCES);
	}
      status = pop_writeline (mpd, "PASS %s\r\n", mpd->passwd);
      CHECK_ERROR_CLOSE (mbox, mpd, status);
      /* We have to nuke the passwd.  */
      memset (mpd->passwd, 0, strlen (mpd->passwd));
      free (mpd->passwd);
      mpd->passwd = NULL;
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, "PASS *\n");
      mpd->state = POP_PASS_TX;

    case POP_PASS_TX:
      /* Send passwd.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      /* We have to nuke the passwd.  */
      memset (mpd->buffer, 0, mpd->buflen);
      mpd->state = POP_PASS_ACK;

    case POP_PASS_ACK:
      /* Get the ack from passwd.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      if (strncasecmp (mpd->buffer, "+OK", 3) != 0)
	{
	  CHECK_ERROR_CLOSE (mbox, mpd, EACCES);
	}
      break;  /* We're outta here.  */

    default:
      /*
	fprintf (stderr, "pop_open unknown state\n");
      */
      break;
    }/* End AUTHORISATION state. */

  /* Spawn cleanup functions.  */
  auth_epilogue (mbox->auth);

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

  /* Flag busy ?  */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;

  /*  Ok boys, it's a wrap: UPDATE State.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      /* Initiate the close.  */
      status = pop_writeline (mpd, "QUIT\r\n");
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_QUIT_TX;

    case POP_QUIT_TX:
      /* Send the quit.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_QUIT_ACK;

    case POP_QUIT_ACK:
      /* Glob the acknowledge.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
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

  /* See if we have already this message.  */
  for (i = 0; i < mpd->pmessages_count; i++)
    {
      if (mpd->pmessages[i])
	{
	  if (mpd->pmessages[i]->num == msgno)
	    {
	      *pmsg = mpd->pmessages[i]->message;
	      return 0;
	    }
	}
    }

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
	|| (status = stream_create (&stream, MU_STREAM_READ, mpm)) != 0)
      {
	message_destroy (&msg, mpm);
	stream_destroy (&stream, mpm);
	free (mpm);
	return status;
      }
    stream_set_read (stream, pop_read_message, mpm);
    stream_set_fd (stream, pop_get_fd, mpm);
    stream_set_flags (stream, MU_STREAM_READ, mpm);
    message_set_stream (msg, stream, mpm);
  }

  /* Create the header.  */
  {
    header_t header = NULL;
    stream_t stream = NULL;
    if ((status = header_create (&header, NULL, 0,  mpm)) != 0
	|| (status = stream_create (&stream, MU_STREAM_READ, mpm)) != 0)
      {
	header_destroy (&header, mpm);
	stream_destroy (&stream, mpm);
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    //stream_set_read (stream, pop_top, mpm);
    stream_set_read (stream, pop_read_header, mpm);
    stream_set_fd (stream, pop_get_fd, mpm);
    stream_set_flags (stream, MU_STREAM_READ, mpm);
    header_set_stream (header, stream, mpm);
    message_set_header (msg, header, mpm);
  }

  /* Create the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, mpm);
    if (status != 0)
      {
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    attribute_set_get_flags (attribute, pop_get_flags, mpm);
    message_set_attribute (msg, attribute, mpm);
  }

  /* Create the body and its stream.  */
  {
    body_t body = NULL;
    stream_t stream = NULL;
    if ((status = body_create (&body, mpm)) != 0
	|| (status = stream_create (&stream, MU_STREAM_READ, mpm)) != 0)
      {
	body_destroy (&body, mpm);
	stream_destroy (&stream, mpm);
	message_destroy (&msg, mpm);
	free (mpm);
	return status;
      }
    stream_set_read (stream, pop_read_body, mpm);
    stream_set_fd (stream, pop_get_fd, mpm);
    stream_set_flags (stream, mpd->flags, mpm);
    body_set_size (body, pop_body_size, mpm);
    body_set_lines (body, pop_body_lines, mpm);
    body_set_stream (body, stream, mpm);
    message_set_body (msg, body, mpm);
  }

  /* Set the UIDL call on the message. */
  message_set_uidl (msg, pop_uidl, mpm);

  /* Add it to the list.  */
  {
    pop_message_t *m ;
    m = realloc (mpd->pmessages, (mpd->pmessages_count + 1)*sizeof (*m));
    if (m == NULL)
      {
	message_destroy (&msg, mpm);
	free (mpm);
	return ENOMEM;
      }
    mpd->pmessages = m;
    mpd->pmessages[mpd->pmessages_count] = mpm;
    mpd->pmessages_count++;
  }

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
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;

  /* TRANSACTION state.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      status = pop_writeline (mpd, "STAT\r\n");
      CHECK_ERROR (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_STAT_TX;

    case POP_STAT_TX:
      /* Send the STAT.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_STAT_ACK;

    case POP_STAT_ACK:
      /* Get the ACK.  */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
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

/* We do not send anything to the server since, no DELE command is sent but
   the only the attribute flag is modified.
   DELE is sent only when expunging.  */
static int
pop_num_deleted (mailbox_t mbox, size_t *pnum)
{
  pop_data_t mpd = mbox->data;
  size_t i, total;
  attribute_t attr;

  if (mpd == NULL)
    return EINVAL;

  for (i = total = 0; i < mpd->messages_count; i++)
    {
      if (message_get_attribute (mpd->pmessages[i]->message, &attr) != 0)
	{
	  if (attribute_is_deleted (attr))
	    total++;
	}
    }

  if (pnum)
    *pnum = total;
  return 0;
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
  for (i = msgno; i <= *pcount; i++)
    if (mailbox_notification (mbox, MU_EVT_MBX_MSG_ADD) != 0)
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
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;

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
		  mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
		  mpd->state = POP_DELE_TX;

		case POP_DELE_TX:
		  /* Send DELETE.  */
		  status = pop_write (mpd);
		  CHECK_EAGAIN (mpd, status);
		  mpd->state = POP_DELE_ACK;

		case POP_DELE_ACK:
		  status = pop_read_ack (mpd);
		  CHECK_EAGAIN (mpd, status);
		  mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
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
  mpd->id = 0;
  mpd->func = NULL;
  mpd->state = POP_NO_STATE;
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

static int
pop_body_size (body_t body, size_t *psize)
{
  pop_message_t mpm = body->owner;
  if (mpm == NULL)
    return EINVAL;
  if (psize)
    *psize = mpm->body_size;
  return 0;
}

static int
pop_body_lines (body_t body, size_t *plines)
{
  pop_message_t mpm = body->owner;
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
pop_get_flags (attribute_t attr, int *pflags)
{
  pop_message_t mpm = attr->owner;
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

static int
pop_get_fd (stream_t stream, int *pfd)
{
  pop_message_t mpm = stream->owner;
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
  pop_message_t mpm = msg->owner;
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
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;

  /* Get the UIDL.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      status = pop_writeline (mpd, "UIDL %d\r\n", mpm->num);
      CHECK_ERROR (mpd, status);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_UIDL_TX;

    case POP_UIDL_TX:
      /* Send the UIDL.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_UIDL_ACK;

    case POP_UIDL_ACK:
      /* Resp from UIDL. */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      break;

    default:
      /*
	fprintf (stderr, "pop_uidl state\n");
      */
      break;
    }

  CLEAR_STATE (mpd);

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
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  size_t nread = 0;
  int status = 0;
  void *func = (void *)pop_top;

  if (mpm == NULL)
    return EINVAL;

  /* We do not carry the offset(for pop), should be doc somewhere.  */
  (void)offset;
  mpd = mpm->mpd;

  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;

  /* Get the header.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      /* TOP is an optionnal command, if we want to be compliant we can not
	 count on it to exists. So we should be prepare when it fails and
	 fall to a second scheme.  */
      status = pop_writeline (mpd, "TOP %d 0\r\n", mpm->num);
      CHECK_ERROR (mpd, status);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      mpd->state = POP_TOP_TX;

    case POP_TOP_TX:
      /* Send the TOP.  */
      status = pop_write (mpd);
      CHECK_EAGAIN (mpd, status);
      mpd->state = POP_TOP_ACK;

    case POP_TOP_ACK:
      /* Ack from TOP. */
      status = pop_read_ack (mpd);
      CHECK_EAGAIN (mpd, status);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
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
      /* Do we need to fill up.  */
      if (mpd->nl == NULL || mpd->ptr == mpd->buffer)
	{
	  status = pop_readline (mpd);
	  CHECK_EAGAIN (mpd, status);
	}
      nread = fill_buffer (mpd, buffer, buflen);
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
pop_read_header (stream_t is, char *buffer, size_t buflen, off_t offset,
		 size_t *pnread)
{
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  void *func = (void *)pop_read_header;

  if (mpm == NULL)
    return EINVAL;
  mpd = mpm->mpd;

  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;

  mpm->skip_header = 0;
  mpm->skip_body = 1;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

/* Stub to call pop_retr ().  */
static int
pop_read_body (stream_t is, char *buffer, size_t buflen, off_t offset,
	       size_t *pnread)
{
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  void *func = (void *)pop_read_body;

  if (mpm == NULL)
    return EINVAL;
  mpd = mpm->mpd;

  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;

  mpm->skip_header = 1;
  mpm->skip_body = 0;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

/* Stub to call pop_retr ().  */
static int
pop_read_message (stream_t is, char *buffer, size_t buflen, off_t offset,
	       size_t *pnread)
{
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  void *func = (void *)pop_read_message;

  if (mpm == NULL)
    return EINVAL;
  mpd = mpm->mpd;

  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;

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
      memcpy (buffer, mpd->buffer, nread);
      memmove (mpd->buffer, mpd->buffer + nread, sentinel);
      mpd->ptr = mpd->buffer + sentinel;
    }
  else
    {
      /* Drain our buffer.  */;
      nread = n;
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

  if (pnread)
    *pnread = nread;
  /*  Take care of the obvious.  */
  if (buffer == NULL || buflen == 0)
    return 0;

  mpd = mpm->mpd;

  switch (mpd->state)
    {
    case POP_NO_STATE:
      mpm->body_lines = mpm->body_size = 0;
      status = pop_writeline (mpd, "RETR %d\r\n", mpm->num);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, mpd->buffer);
      CHECK_ERROR (mpd, status);
      mpd->state = POP_RETR_TX;

    case POP_RETR_TX:
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
	    }
	  /* Oops !! Hello houston we have a major problem here.  */
	  if (mpd->buffer[0] == '\0')
	    {
	      /* Still Do the right thing.  */
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
	  if (mpm->skip_header == 0)
	    {
	      nread = fill_buffer (mpd, buffer, buflen);
	      if (pnread)
		*pnread += nread;
	      buflen -= nread ;
              if (buflen > 0)
                buffer += nread;
              else
                return 0;
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

	    if (mpm->skip_body == 0)
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
	    else
	      {
		mpm->body_size += (mpd->ptr - mpd->buffer);
		mpd->ptr = mpd->buffer;
	      }
	  }
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
