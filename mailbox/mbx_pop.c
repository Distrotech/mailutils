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

#include <mailbox0.h>
#include <stream0.h>
#include <body0.h>
#include <message0.h>
#include <registrar0.h>
#include <auth0.h>
#include <header0.h>
#include <attribute0.h>

/* Advance declarations.  */
struct _pop_data;
struct _pop_message;
struct _bio;

typedef struct _pop_data * pop_data_t;
typedef struct _pop_message * pop_message_t;
typedef struct _bio *bio_t;

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
static int fill_buffer (bio_t bio, char *buffer, size_t buflen);

/* According to the rfc:
   RFC 2449                POP3 Extension Mechanism           November 1998

   4.  Parameter and Response Lengths

   This specification increases the length restrictions on commands and
   parameters imposed by RFC 1939.

   The maximum length of a command is increased from 47 characters (4
   character command, single space, 40 character argument, CRLF) to 255
   octets, including the terminating CRLF.

   Servers which support the CAPA command MUST support commands up to
   255 octets.  Servers MUST also support the largest maximum command
   length specified by any supported capability.

   The maximum length of the first line of a command response (including
   the initial greeting) is unchanged at 512 octets (including the
   terminating CRLF).  */

/* Buffered I/O, since the connection maybe non-blocking, we use a little
   working buffer to hold the I/O between us and the POP3 Server.  For
   example if write () return EAGAIN, we keep on calling bio_write () until
   the buffer is empty. bio_read () does a little more, it takes care of
   filtering out the starting "." and return O(zero) when seeing the terminal
   octets ".\r\n".  bio_readline () insists on having a _complete_ line .i.e
   a string terminated by \n, it will allocate/grow the working buffer as
   needed.  The '\r\n" termination is converted to '\n'.  bio_destroy ()
   does not close the stream but only free () the working buffer.  */
struct _bio
{
#define POP_BUFSIZ 512
  stream_t stream;
  size_t maxlen;
  size_t len;
  char *current;
  char *buffer;
  char *ptr;
  char *nl;
};

static int  bio_create   (bio_t *, stream_t);
static void bio_destroy  (bio_t *);
static int  bio_readline (bio_t);
static int  bio_read     (bio_t);
static int  bio_write    (bio_t);


/*  This structure holds the info when for a pop_get_message() the
    pop_message_t type  will serve as the owner of the message_t and contains
    the command to send to RETReive the specify message.  The problem comes
    from the header.  If the  POP server supports TOP, we can cleanly fetch
    the header.  But otherwise we use the clumsy approach. .i.e for the header
    we read 'til ^\n then discard the rest for the body we read after ^\n and
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
  size_t id;  /* Use in pop_expunge to indiate the message, if bailing out.  */
  enum pop_state state;
  pop_message_t *pmessages;
  size_t pmessages_count;
  size_t messages_count;
  size_t size;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  int flags;  /* Flags of for the stream_t object.  */
  bio_t bio;  /* Working I/O buffer.  */
  int is_updated;
  char *user;  /*  Temporary holders for user and passwd.  */
  char *passwd;
  mailbox_t mbox; /* Back pointer.  */
} ;

/* Little Macro, since this is very repetitive.  */
#define CLEAR_STATE(mpd) \
 mpd->func = NULL, \
 mpd->state = POP_NO_STATE

/* Clear the state for non recoverable error.  */
#define CHECK_NON_RECOVERABLE(mpd, status) \
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


/* Parse the url, allocate mailbox_t etc .. */
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

/*  Cleaning up all the ressources.  */
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
		}
	    }
	  bio_destroy (&(mpd->bio));
	  free (mpd->pmessages);
	  free (mpd);
#ifdef HAVE_PHTREAD_H
	  pthread_mutex_destroy (&(mpd->mutex));
#endif
	}
      /* Since the mailbox is destroy, close the stream but not via
	 pop_close () which can return EAGAIN, or other unpleasant side
	 effects, but rather the hard way.  */
      stream_close (mbox->stream);
      stream_destroy (&(mbox->stream), mbox);
      auth_destroy (&(mbox->auth), mbox);
      if (mbox->name)
	free (mbox->name);
      if (mbox->event)
	free (mbox->event);
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

static int
pop_open (mailbox_t mbox, int flags)
{
  pop_data_t mpd = mbox->data;
  int status;
  bio_t bio;
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
  bio = mpd->bio;

  /* Enter the pop state machine, and boogy: AUTHORISATION State.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      /* Spawn auth prologue.  */
      auth_prologue (mbox->auth);
      /* Create the networking stack.  */
      if (mbox->stream == NULL)
	{
	  status = tcp_stream_create (&(mbox->stream));
	  if (status != 0)
	    return status;
	}
      mpd->state = POP_OPEN_CONNECTION;

    case POP_OPEN_CONNECTION:
      /* Establish the connection.  */
      status = stream_open (mbox->stream, host, port, flags);
      if (status != 0)
	{
	  /* Clear the state for non recoverable error.  */
	  if (status != EAGAIN && status != EINPROGRESS && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = POP_NO_STATE;
	    }
	  return status;
	}
      /* Allocate the struct for buffered I/O.  */
      bio_destroy (&(mpd->bio));
      status = bio_create (&(mpd->bio), mbox->stream);
      /* Can't recover bailout.  */
      if (status != 0)
	{
	  stream_close (bio->stream);
	  CLEAR_STATE (mpd);
	  return status;
	}
      bio = mpd->bio;
      mpd->state = POP_GREETINGS;

    case POP_GREETINGS:
      /* Swallow the greetings.  */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	{
	  stream_close (bio->stream);
	  CLEAR_STATE (mpd);
	  return EACCES;
	}

      /*  Fetch the the user/passwd from them.  */
      auth_authenticate (mbox->auth, &mpd->user, &mpd->passwd);

      bio->len = snprintf (bio->buffer, bio->maxlen, "USER %s\r\n", mpd->user);
      bio->ptr = bio->buffer;
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      free (mpd->user); mpd->user = NULL;
      mpd->state = POP_USER_TX;

    case POP_USER_TX:
      /* Send username.  */
      status = bio_write (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_USER_ACK;

    case POP_USER_ACK:
      /* Get the user ack.  */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	{
	  stream_close (bio->stream);
	  CLEAR_STATE (mpd);
	  return EACCES;
	}
      bio->len = snprintf (bio->buffer, POP_BUFSIZ, "PASS %s\r\n",
			   mpd->passwd);
      bio->ptr = bio->buffer;
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      /* We have to nuke the passwd.  */
      memset (mpd->passwd, 0, strlen (mpd->passwd));
      free (mpd->passwd); mpd->passwd = NULL;
      mpd->state = POP_PASS_TX;

    case POP_PASS_TX:
      /* Send passwd.  */
      status = bio_write (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_PASS_ACK;

    case POP_PASS_ACK:
      /* Get the ack from passwd.  */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	{
	  stream_close (bio->stream);
	  CLEAR_STATE (mpd);
	  return EACCES;
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

static int
pop_close (mailbox_t mbox)
{
  pop_data_t mpd = mbox->data;
  void *func = (void *)pop_close;
  int status;
  bio_t bio;
  size_t i;

  if (mpd == NULL)
    return EINVAL;

  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;

  /*  Ok boys, it's a wrap: UPDATE State.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      bio->len = snprintf (bio->buffer, bio->maxlen, "QUIT\r\n");
      bio->ptr = bio->buffer;
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      mpd->state = POP_QUIT_TX;

    case POP_QUIT_TX:
      /* Send the quit.  */
      status = bio_write (mpd->bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_QUIT_ACK;

    case POP_QUIT_ACK:
      /* Glob the acknowledge.  */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      /*  Now what ! and how can we tell them about errors ?  So for now
	  lets just be verbose about the error but close the connection
	  anyway.  */
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	fprintf (stderr, "pop_close: %s\n", bio->buffer);

      stream_close (bio->stream);
      bio->stream = NULL;
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
	}
    }
  free (mpd->pmessages);
  mpd->pmessages = NULL;
  mpd->pmessages_count = 0;
  mpd->is_updated = 0;
  bio_destroy (&(mpd->bio));

  CLEAR_STATE (mpd);
  return 0;
}

static int
pop_get_message (mailbox_t mbox, size_t msgno, message_t *pmsg)
{
  pop_data_t mpd = mbox->data;
  int status;
  pop_message_t mpm;
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
  /* back pointer */
  mpm->mpd = mpd;
  mpm->num = msgno;

  /* Create the message.  */
  {
    message_t msg;
    stream_t is;
    if ((status = message_create (&msg, mpm)) != 0
	|| (status = stream_create (&is, MU_STREAM_READ, mpm)) != 0)
      {
	free (mpm);
	return status;
      }
    stream_set_read (is, pop_read_message, mpm);
    message_set_stream (msg, is, mpm);
    /* The message.  */
    mpm->message = msg;
  }

  /* Create the header.  */
  {
    header_t header;
    stream_t stream;
    if ((status = header_create (&header, NULL, 0,  mpm)) != 0
	|| (status = stream_create (&stream, MU_STREAM_READ, mpm)) != 0)
      {
	message_destroy (&(mpm->message), mpm);
	free (mpm);
	return status;
      }
    //stream_set_read (stream, pop_read_header, mpm);
    stream_set_read (stream, pop_top, mpm);
    stream_set_fd (stream, pop_get_fd, mpm);
    stream_set_flags (stream, MU_STREAM_READ, mpm);
    header_set_stream (header, stream, mpm);
    message_set_header (mpm->message, header, mpm);
  }

  /* Create the attribute.  */
  {
    attribute_t attribute;
    status = attribute_create (&attribute, mpm);
    if (status != 0)
      {
	message_destroy (&(mpm->message), mpm);
	free (mpm);
	return status;
      }
    attribute_set_get_flags (attribute, pop_get_flags, mpm);
    message_set_attribute (mpm->message, attribute, mpm);
  }

  /* Create the body and its stream.  */
  {
    stream_t stream;
    body_t body;
    if ((status = body_create (&body, mpm)) != 0
	|| (status = stream_create (&stream, MU_STREAM_READ, mpm)) != 0)
      {
	message_destroy (&(mpm->message), mpm);
	free (mpm);
	return status;
      }
    stream_set_read (stream, pop_read_body, mpm);
    stream_set_fd (stream, pop_get_fd, mpm);
    stream_set_flags (stream, mpd->flags, mpm);
    body_set_size (body, pop_body_size, mpm);
    body_set_lines (body, pop_body_lines, mpm);
    body_set_stream (body, stream, mpm);
    message_set_body (mpm->message, body, mpm);
  }

  /* Set the UIDL call on the message. */
  message_set_uidl (mpm->message, pop_uidl, mpm);

  /* Add it to the list.  */
  {
    pop_message_t *m ;
    m = realloc (mpd->pmessages, (mpd->pmessages_count + 1)*sizeof (*m));
    if (m == NULL)
      {
	message_destroy (&(mpm->message), mpm);
	free (mpm);
	return ENOMEM;
      }
    mpd->pmessages = m;
    mpd->pmessages[mpd->pmessages_count] = mpm;
    mpd->pmessages_count++;
  }

  *pmsg = mpm->message;

  return 0;
}

static int
pop_messages_count (mailbox_t mbox, size_t *pcount)
{
  pop_data_t mpd = mbox->data;
  int status;
  void *func = (void *)pop_messages_count;
  bio_t bio;

  if (mpd == NULL)
    return EINVAL;

  if (pop_is_updated (mbox))
    {
      if (pcount)
	*pcount = mpd->messages_count;
      return 0;
    }

  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;

  /* TRANSACTION state.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      bio->len = sprintf (bio->buffer, "STAT\r\n");
      bio->ptr = bio->buffer;
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      mpd->state = POP_STAT_TX;

    case POP_STAT_TX:
      /* Send the STAT.  */
      status = bio_write (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_STAT_ACK;

    case POP_STAT_ACK:
      /* Get the ACK.  */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      break;
    default:
      /*
	fprintf (stderr, "pomp_messages_count: unknow state\n");
      */
      break;
    }

  status = sscanf (bio->buffer, "+OK %d %d", &(mpd->messages_count),
		   &(mpd->size));

  CLEAR_STATE (mpd);

  mpd->is_updated = 1;
  if (pcount)
    *pcount = mpd->messages_count;
  if (status == EOF || status != 2)
    return EIO;
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

/* We just simulated.  */
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

/* This were we actually sending the DELE command.  */
static int
pop_expunge (mailbox_t mbox)
{
  pop_data_t mpd = mbox->data;
  size_t i;
  attribute_t attr;
  bio_t bio;
  int status;
  void *func = (void *)pop_expunge;

  if (mpd == NULL)
    return EINVAL;

  /* Busy ?  */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;

  for (i = (int)mpd->id; i < mpd->pmessages_count; mpd->id = ++i)
    {
      if (message_get_attribute (mpd->pmessages[i]->message, &attr) == 0)
	{
	  if (attribute_is_deleted (attr))
	    {
	      switch (mpd->state)
		{
		case POP_NO_STATE:
		  bio->len = sprintf (bio->buffer, "DELE %d\r\n",
				      mpd->pmessages[i]->num);
		  bio->ptr = bio->buffer;
		  mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
		  mpd->state = POP_DELE_TX;

		case POP_DELE_TX:
		  /* Send DELETE.  */
		  status = bio_write (bio);
		  if (status != 0)
		    {
		      /* Clear the state for non recoverable error.  */
		      if (status != EAGAIN && status != EINTR)
			{
			  mpd->func = NULL;
			  mpd->id = 0;
			  mpd->state = POP_NO_STATE;
			}
		      return status;
		    }
		  mpd->state = POP_DELE_ACK;

		case POP_DELE_ACK:
		  status = bio_readline (bio);
		  if (status != 0)
		    {
		      /* Clear the state for non recoverable error.  */
		      if (status != EAGAIN && status != EINTR)
			{
			  mpd->func = NULL;
			  mpd->id = 0;
			  mpd->state = POP_NO_STATE;
			}
		      return status;
		    }
		  mailbox_debug (mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
		  if (strncasecmp (bio->buffer, "+OK", 3) != 0)
		    {
		      mpd->func = NULL;
		      mpd->id = 0;
		      mpd->state = POP_NO_STATE;
		      return ERANGE;
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
  /* Invalidate.  But Really they should shutdown the challen POP protocol
     is not meant for this like IMAP.  */
  mpd->is_updated = 0;
  return 0;
}

/* Mailbox size ? */
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
  if (mpm == NULL)
    return EINVAL;
  if (mpm->mpd->bio)
    return stream_get_fd (mpm->mpd->bio->stream, pfd);
  return EINVAL;
}

static int
pop_uidl (message_t msg, char *buffer, size_t buflen, size_t *pnwriten)
{
  pop_message_t mpm = msg->owner;
  pop_data_t mpd;
  bio_t bio;
  int status = 0;
  void *func = (void *)pop_uidl;
  size_t num;
  /* According to the RFC uidl's are no longer then 70 chars.  */
  char uniq[128];

  if (mpm == NULL)
    return EINVAL;

  mpd = mpm->mpd;
  bio = mpd->bio;

  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;

  /* Get the UIDL.  */
  switch (mpd->state)
    {
    case POP_NO_STATE:
      bio->len = sprintf (bio->buffer, "UIDL %d\r\n", mpm->num);
      bio->ptr = bio->buffer;
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      mpd->state = POP_UIDL_TX;

    case POP_UIDL_TX:
      /* Send the UIDL.  */
      status = bio_write (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_UIDL_ACK;

    case POP_UIDL_ACK:
      /* Resp from TOP. */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      break;
    default:
      /*
	fprintf (stderr, "pop_uidl state\n");
      */
      break;
    }

  CLEAR_STATE (mpd);

  *uniq = '\0';
  status = sscanf (bio->buffer, "+OK %d %127s\n", &num, uniq);
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

static int
pop_top (stream_t is, char *buffer, size_t buflen,
		 off_t offset, size_t *pnread)
{
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  bio_t bio;
  size_t nread = 0;
  int status = 0;
  void *func = (void *)pop_top;

  if (mpm == NULL)
    return EINVAL;

  /* We do not carry the offset(for pop), should be doc somewhere.  */
  (void)offset;
  mpd = mpm->mpd;
  bio = mpd->bio;

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
      bio->len = sprintf (bio->buffer, "TOP %d 0\r\n", mpm->num);
      bio->ptr = bio->buffer;
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      mpd->state = POP_TOP_TX;

    case POP_TOP_TX:
      /* Send the TOP.  */
      status = bio_write (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_TOP_ACK;

    case POP_TOP_ACK:
      /* Ack from TOP. */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mailbox_debug (mpd->mbox, MU_MAILBOX_DEBUG_PROT, bio->buffer);
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	{
	  /* fprintf (stderr, "TOP not implmented\n"); */
	  /* Fall back to RETR call.  */
	  mpd->state = POP_NO_STATE;
	  mpm->skip_header = 0;
	  mpm->skip_body = 1;
	  return pop_retr (mpm, buffer, buflen, offset, pnread);
	}
      mpd->state = POP_TOP_RX;
      bio->current = bio->nl;

    case POP_TOP_RX:
      /* Get the header.  */
      {
	int nleft, n;
	/* Do we need to fill up.  */
	if (bio->current >= bio->nl)
	  {
	    bio->current = bio->buffer;
	    status = bio_readline (bio);
	    CHECK_NON_RECOVERABLE (mpd, status);
	  }
	n = bio->nl - bio->current;
	nleft = buflen - n;
	/* We got more then requested.  */
	if (nleft <= 0)
	  {
	    memcpy (buffer, bio->current, buflen);
	    bio->current += buflen;
	    nread = buflen;
	  }
	else
	  {
	    /* Drain the buffer.  */
	    memcpy (buffer, bio->current, n);
	    bio->current += n;
	    nread = n;
	  }
	break;
      }
      break;
    default:
      /* Probabaly TOP was not supported so we fall back to RETR.  */
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

static int
pop_read_header (stream_t is, char *buffer, size_t buflen, off_t offset,
		 size_t *pnread)
{
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  void *func = (void *)pop_read_header;

  if (mpm == NULL)
    return EINVAL;
  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;

  mpm->skip_header = 0;
  mpm->skip_body = 1;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

static int
pop_read_body (stream_t is, char *buffer, size_t buflen, off_t offset,
	       size_t *pnread)
{
  pop_message_t mpm = is->owner;
  pop_data_t mpd;
  void *func = (void *)pop_read_body;

  if (mpm == NULL)
    return EINVAL;
  /* Busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;

  mpm->skip_header = 1;
  mpm->skip_body = 0;
  return pop_retr (mpm, buffer, buflen, offset, pnread);
}

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

static int
fill_buffer (bio_t bio, char *buffer, size_t buflen)
{
  int nleft, n, nread = 0;
  n = bio->nl - bio->current;
  nleft = buflen - n;
  /* We got more then requested.  */
  if (nleft <= 0)
    {
      memcpy (buffer, bio->current, buflen);
      bio->current += buflen;
      nread = buflen;
    }
  else
    {
      /* Drain the buffer.  */
      memcpy (buffer, bio->current, n);
      bio->current += n;
      nread = n;
    }
  return nread;
}

static int
pop_retr (pop_message_t mpm, char *buffer, size_t buflen, off_t offset,
	  size_t *pnread)
{
  pop_data_t mpd;
  size_t nread = 0;
  bio_t bio;
  int status = 0;

  (void)offset;

  if (pnread)
    *pnread = nread;
  /*  Take care of the obvious.  */
  if (buffer == NULL || buflen == 0)
    return 0;

  mpd = mpm->mpd;
  bio = mpd->bio;

  switch (mpd->state)
    {
    case POP_NO_STATE:
      mpm->body_lines = mpm->body_size = 0;
      bio->len = sprintf (bio->buffer, "RETR %d\r\n", mpm->num);
      bio->ptr = bio->buffer;
      mpd->state = POP_RETR_TX;

    case POP_RETR_TX:
      /* Send the RETR command.  */
      status = bio_write (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      mpd->state = POP_RETR_ACK;

    case POP_RETR_ACK:
      /* RETR ACK.  */
      status = bio_readline (bio);
      CHECK_NON_RECOVERABLE (mpd, status);
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	{
	  CLEAR_STATE (mpd);
	  return EACCES;
	}
      bio->current = bio->nl;
      mpd->state = POP_RETR_RX_HDR;

    case POP_RETR_RX_HDR:
      /* Skip the header.  */
      while (!mpm->inbody)
	{
	  /* Do we need to fill up.  */
	  if (bio->current >= bio->nl)
	    {
	      bio->current = bio->buffer;
	      status = bio_readline (bio);
	      CHECK_NON_RECOVERABLE (mpd, status);
	    }
	  if (mpm->skip_header == 0)
	    {
	      nread = fill_buffer (bio, buffer, buflen);
	      if (pnread)
		*pnread += nread;
	      buflen -= nread ;
	      if (buflen > 0)
		buffer += nread;
	      else
		return 0;
	    }
	  else
	    bio->current = bio->nl;
	  if (bio->buffer[0] == '\n')
	    {
	      mpm->inbody = 1;
	      break;
	    }
	}
      /* Skip the newline.  */
      //bio_readline (bio);
      mpd->state = POP_RETR_RX_BODY;

    case POP_RETR_RX_BODY:
      /* Start taking the body.  */
      {
	while (mpm->inbody)
	  {
	    /* Do we need to fill up.  */
	    if (bio->current >= bio->nl)
	      {
		bio->current = bio->buffer;
		status = bio_readline (bio);
		CHECK_NON_RECOVERABLE (mpd, status);
		mpm->body_lines++;
	      }
	    if (mpm->skip_body == 0)
	      {
		nread = fill_buffer (bio, buffer, buflen);
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
		mpm->body_size += (bio->nl - bio->current);
		bio->current = bio->nl;
	      }
	    if (bio->buffer[0] == '\0')
	      {
		mpm->inbody = 0;
		break;
	      }
	  }
      }
      mpd->state = POP_STATE_DONE;
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
bio_create (bio_t *pbio, stream_t stream)
{
  bio_t bio;
  bio = calloc (1, sizeof (*bio));
  if (bio == NULL)
    return ENOMEM;
  bio->maxlen = POP_BUFSIZ + 1;
  bio->current = bio->ptr = bio->buffer = calloc (1, bio->maxlen);
  bio->stream = stream;
  *pbio = bio;
  return 0;
}

static void
bio_destroy (bio_t *pbio)
{
  if (pbio && *pbio)
    {
      bio_t bio = *pbio;
      free (bio->buffer);
      free (bio);
      *pbio = NULL;
    }
}

static int
bio_write (bio_t bio)
{
  size_t nwritten = 0;
  int status;

  while (bio->len > 0)
    {
      status = stream_write (bio->stream, bio->ptr, bio->len, 0, &nwritten);
      if (status != 0)
	return status;
      bio->len -= nwritten;
      bio->ptr += nwritten;
    }

  bio->ptr = bio->buffer;
  return 0;
}

static int
bio_read (bio_t bio)
{
  size_t nread = 0;
  int status, len;

  len = bio->maxlen - (bio->ptr - bio->buffer) - 1;
  status = stream_read (bio->stream, bio->ptr, len, 0, &nread);
  if (status != 0)
    return status;
  else if (nread == 0) /* EOF ???? We got a situation here ??? */
    {
      bio->buffer[0] = '.';
      bio->buffer[1] = '\r';
      bio->buffer[2] = '\n';
      nread = 3;
    }
  bio->ptr[nread] = '\0';
  bio->ptr += nread;
  return 0;
}

/* Responses to certain commands are multi-line.  In these cases, which
   are clearly indicated below, after sending the first line of the
   response and a CRLF, any additional lines are sent, each terminated
   by a CRLF pair.  When all lines of the response have been sent, a
   final line is sent, consisting of a termination octet (decimal code
   046, ".") and a CRLF pair.  */
static int
bio_readline (bio_t bio)
{
  size_t len;
  int status;

  /* Still unread lines in the buffer ? */
  if (bio->nl && bio->nl < bio->ptr)
    {
      memmove (bio->buffer, bio->nl + 1, bio->ptr - bio->nl);
      bio->ptr = bio->buffer + (bio->ptr - bio->nl) - 1;
      bio->nl = memchr (bio->buffer, '\n', bio->ptr - bio->buffer + 1);
    }
  else
    bio->nl = bio->ptr = bio->buffer;

  if (bio->nl == NULL || bio->nl == bio->ptr)
    {
      for (;;)
	{
	  status = bio_read (bio);
	  if (status != 0)
	    return status;
	  len = bio->ptr  - bio->buffer;
	  /* Newline.  */
	  bio->nl = memchr (bio->buffer, '\n', len);
	  if (bio->nl == NULL)
	    {
	      if (len >= (bio->maxlen - 1))
		{
		  char *tbuf;
		  tbuf = realloc (bio->buffer,
				  (2*(bio->maxlen) + 2)*(sizeof(char)));
		  if (tbuf == NULL)
		    return ENOMEM;
		  bio->buffer = tbuf;
		  bio->maxlen *= 2;
		}
	      bio->ptr = bio->buffer + len;
	    }
	  else
	    break;
	}
    }

  /* When examining a multi-line response, the client checks
     to see if the line begins with the termination octet "."(DOT).
     If so and if octets other than CRLF follow, the first octet
     of the line (the termination octet) is stripped away.  */
  if (bio->buffer[0] == '.')
    {
      if (bio->buffer[1] != '\r' && bio->buffer[2] != '\n')
	{
	  memmove (bio->buffer, bio->buffer + 1, bio->ptr - bio->buffer);
	  bio->ptr--;
	  bio->nl--;
	}
      /* If so and if CRLF immediately
         follows the termination character, then the response from the POP
         server is ended and the line containing ".CRLF" is not considered
         part of the multi-line response.  */
        else if (bio->buffer[1] == '\r' && bio->buffer[2] == '\n')
	{
	  bio->buffer[0] = '\0';
	  bio->nl = bio->ptr = bio->buffer;
	  return 0;
	}
    }

  /* \r\n --> \n\0  */
  if (bio->nl > bio->buffer)
    {
      *(bio->nl - 1) = '\n';
      *(bio->nl) = '\0';
    }

  return 0;
}
