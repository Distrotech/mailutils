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

#include <mailbox0.h>
#include <io0.h>
#include <body0.h>
#include <message0.h>
#include <registrar0.h>
#include <auth0.h>
#include <attribute.h>

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>


static int mailbox_pop_create (mailbox_t *mbox, const char *name);
static void mailbox_pop_destroy (mailbox_t *mbox);

struct mailbox_registrar _mailbox_pop_registrar =
{
  "POP3",
  mailbox_pop_create, mailbox_pop_destroy
};

static int mailbox_pop_open (mailbox_t, int flags);
static int mailbox_pop_close (mailbox_t);
static int mailbox_pop_get_message (mailbox_t, size_t msgno, message_t *msg);
static int mailbox_pop_messages_count (mailbox_t, size_t *num);
static int mailbox_pop_expunge (mailbox_t);
static int mailbox_pop_num_deleted (mailbox_t, size_t *);

/* update and scanning*/
static int mailbox_pop_is_updated (mailbox_t);
static int mailbox_pop_scan (mailbox_t mbox, size_t msgno, size_t *pcount);

/* mailbox size ? */
static int mailbox_pop_size (mailbox_t, off_t *size);

static int mailbox_pop_readstream (stream_t is, char *buffer, size_t buflen,
				   off_t offset, size_t *pnread);
static int mailbox_pop_getfd (stream_t, int *pfd);
static int mailbox_pop_body_size (body_t, size_t *psize);


/* According to the rfc:
 * RFC 2449                POP3 Extension Mechanism           November 1998

 * 4.  Parameter and Response Lengths

 * This specification increases the length restrictions on commands and
 * parameters imposed by RFC 1939.

 * The maximum length of a command is increased from 47 characters (4
 * character command, single space, 40 character argument, CRLF) to 255
 * octets, including the terminating CRLF.

 * Servers which support the CAPA command MUST support commands up to
 * 255 octets.  Servers MUST also support the largest maximum command
 * length specified by any supported capability.

 * The maximum length of the first line of a command response (including
 * the initial greeting) is unchanged at 512 octets (including the
 * terminating CRLF).
 */

/* buffered IO */
struct _bio {
#define POP_BUFSIZ 512
  int fd;
  size_t maxlen;
  size_t len;
  char *current;
  char *buffer;
  char *ptr;
  char *nl;
};

typedef struct _bio *bio_t;

static int bio_create (bio_t *, int);
static void bio_destroy (bio_t *);
static int bio_readline (bio_t);
static int bio_read (bio_t);
static int bio_write (bio_t);

struct _mailbox_pop_data;
struct _mailbox_pop_message;

typedef struct _mailbox_pop_data * mailbox_pop_data_t;
typedef struct _mailbox_pop_message * mailbox_pop_message_t;

struct _mailbox_pop_message
{
  bio_t bio;
  int inbody;
  size_t num;
  off_t body_size;
  message_t message;
  mailbox_pop_data_t mpd;
};

struct _mailbox_pop_data
{
  void *func;
  void *id;
  int state;
  mailbox_pop_message_t *pmessages;
  size_t pmessages_count;
  size_t messages_count;
  size_t size;
#ifdef HAVE_PTHREAD_H
  pthread_mutex_t mutex;
#endif
  int flags;
  int fd;
  bio_t bio;
  int is_updated;
  char *user;
  char *passwd;
  mailbox_pop_message_t mpm;
} ;


/* Parse the url, allocate mailbox_t etc .. */
static int
mailbox_pop_create (mailbox_t *pmbox, const char *name)
{
  mailbox_t mbox;
  mailbox_pop_data_t mpd;
  size_t name_len;
  int status;

  /* sanity check */
  if (pmbox == NULL || name == NULL || *name == '\0')
    return EINVAL;

  name_len = strlen (name);

#define POP_SCHEME "pop://"
#define POP_SCHEME_LEN 6
#define SEPARATOR '/'

  /* skip the url scheme */
  if (name_len > POP_SCHEME_LEN &&
      (name[0] == 'p' || name[0] == 'P') &&
      (name[1] == 'o' || name[1] == 'O') &&
      (name[2] == 'p' || name[2] == 'P') &&
      (name[3] == ':' && name[4] == '/' && name[5] == '/'))
    {
      name += POP_SCHEME_LEN;
      name_len -= POP_SCHEME_LEN;
    }

  /* allocate memory for mbox */
  mbox = calloc (1, sizeof (*mbox));
  if (mbox == NULL)
    return ENOMEM;

  /* allocate specific pop box data */
  mpd = mbox->data = calloc (1, sizeof (*mpd));
  if (mbox->data == NULL)
    {
      mailbox_pop_destroy (&mbox);
      return ENOMEM;
    }

  /* allocate the struct for buffered I/O */
  status = bio_create (&(mpd->bio), -1);
  if (status != 0)
    {
      mailbox_pop_destroy (&mbox);
      return status;
    }

  /* copy the name */
  mbox->name = calloc (name_len + 1, sizeof (char));
  if (mbox->name == NULL)
    {
      mailbox_pop_destroy (&mbox);
      return ENOMEM;
    }
  memcpy (mbox->name, name, name_len);

#ifdef HAVE_PHTREAD_H
  /* mutex when accessing the structure fields */
  /* FIXME: should we use rdwr locks instead ?? */
  pthread_mutex_init (&(mud->mutex), NULL);
#endif

  /* initialize the structure */
  mbox->_create = mailbox_pop_create;
  mbox->_destroy = mailbox_pop_destroy;

  mbox->_open = mailbox_pop_open;
  mbox->_close = mailbox_pop_close;

  /* messages */
  mbox->_get_message = mailbox_pop_get_message;
  mbox->_messages_count = mailbox_pop_messages_count;
  mbox->_expunge = mailbox_pop_expunge;
  mbox->_num_deleted = mailbox_pop_num_deleted;

  mbox->_scan = mailbox_pop_scan;
  mbox->_is_updated = mailbox_pop_is_updated;

  mbox->_size = mailbox_pop_size;

  (*pmbox) = mbox;

  return 0; /* okdoke */
}

static void
mailbox_pop_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    {
      if ((*pmbox)->data)
	{
	  mailbox_pop_data_t mpd = (*pmbox)->data;
	  size_t i;
	  for (i = 0; i < mpd->pmessages_count; i++)
	    {
	      if (mpd->pmessages[i])
		{
		  bio_destroy (&(mpd->pmessages[i]->bio));
		  message_destroy (&(mpd->pmessages[i]->message),
				   mpd->pmessages[i]);
		}
	      free (mpd->pmessages[i]);
	    }
	  free (mpd->pmessages);
	  free (mpd);
	}
      free ((*pmbox)->name);
      free ((*pmbox)->event);
      if ((*pmbox)->url)
	url_destroy (&((*pmbox)->url));
      *pmbox = NULL;
    }
}

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
  /* FIXME: this incorrect and goes against GNU style: having a limitation
   * on the user/passwd length, it should be fix
   */
  char u[128];
  char p[128];
  mailbox_t mbox = auth->owner;
  int status;

  *u = '\0';
  *p = '\0';

  /* prompt for the user/login name */
  status = url_get_user (mbox->url, u, sizeof (u), NULL);
  if (status != 0 || *u == '\0')
    {
      printf ("Pop User: "); fflush (stdout);
      fgets (u, sizeof (u), stdin);
      u [strlen (u) - 1] = '\0'; /* nuke the trailing NL */
    }
  /* prompt for the passwd */
  /* FIXME: should turn off echo .... */
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
mailbox_pop_open (mailbox_t mbox, int flags)
{
  mailbox_pop_data_t mpd;
  int status;
  bio_t bio;
  void *func = (void *)mailbox_pop_open;
  int fd;
  char host[256]  ;
  long port;

  /* sanity checks */
  if (mbox == NULL || mbox->url == NULL || (mpd = mbox->data) == NULL)
    return EINVAL;

  /* create the networking stack */
  if (!mbox->stream)
    {
      if ((status = url_get_host (mbox->url, host, sizeof(host), NULL)) != 0 ||
	  (status = url_get_port (mbox->url, &port)) != 0 ||
	  (status = tcp_stream_create (&(mbox->stream))) != 0)
	{
	  mpd->func = NULL;
	  mpd->state = 0;
	  return status;
	}
    }

  /* flag busy */
  if (mpd->func && mpd->func != func)
    return EBUSY;
  mpd->func = func;
  bio = mpd->bio;

  /* spawn the prologue */
  if (mbox->auth)
    auth_prologue (mbox->auth);

  /* enter the state machine */
  switch (mpd->state)
    {
      /* establish the connection */
    case 0:
      status = stream_open (mbox->stream, host, port, flags);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINPROGRESS && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      /* get the fd */
      stream_get_fd (mbox->stream, &fd);
      mpd->bio->fd = mpd->fd = fd;
      mpd->state = 1;
      /* glob the Greetings */
    case 1:
      status = bio_readline (bio);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	{
	  mpd->func = NULL;
	  mpd->state = 0;
	  close (mpd->fd);
	  mpd->bio->fd = -1;
	  return EACCES;
	}
      /* Dealing whith Authentication */
      /* so far only normal user/pass supported */
      if (mbox->auth == NULL)
	{
	  status = auth_create (&(mbox->auth), mbox);
	  if (status != 0)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	      return status;
	    }
	  auth_set_authenticate (mbox->auth, pop_authenticate, mbox);
	}

      auth_authenticate (mbox->auth, &mpd->user, &mpd->passwd);
      /* FIXME use snprintf */
      //mpd->len = sprintf (pop->buffer, POP_BUFSIZ, "USER %s\r\n", user);
      bio->len = sprintf (bio->buffer, "USER %s\r\n", mpd->user);
      bio->ptr = bio->buffer;
      free (mpd->user); mpd->user = NULL;
      mpd->state = 2;
      /* send username */
    case 2:
      status = bio_write (bio);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      mpd->state = 3;
      /* get the ack */
    case 3:
      status = bio_readline (bio);
      if (status  != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	return EACCES;

      /* FIXME use snprintf */
      //mpd->len = snprintf (mpd->buffer, POP_BUFSIZ, "PASS %s\r\n", passwd);
      bio->len = sprintf (bio->buffer, "PASS %s\r\n", mpd->passwd);
      bio->ptr = bio->buffer;
      free (mpd->passwd); mpd->passwd = NULL;
      mpd->state = 4;
      /* send Passwd */
    case 4:
      status = bio_write (bio);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      mpd->state = 5;
      /* get the ack from passwd */
    case 5:
      status = bio_readline (bio);
      if (status  != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	return EACCES;
    }/* swith state */

  /* spawn cleanup functions */
  if (mbox->auth)
    auth_epilogue (mbox->auth);

  /* clear any state */
  mpd->func = NULL;
  mpd->state = 0;

  return 0;
}

static int
mailbox_pop_close (mailbox_t mbox)
{
  mailbox_pop_data_t mpd;
  void *func = (void *)mailbox_pop_close;
  int status;
  bio_t bio;

  if (mbox == NULL || (mpd = mbox->data) == NULL)
    return EINVAL;

  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;

  if (mpd->fd != -1)
    {
      switch (mpd->state)
	{
	case 0:
	  bio->len = sprintf (bio->buffer, "QUIT\r\n");
	  bio->ptr = bio->buffer;
	  mpd->state = 1;
	case 1:
	  status = bio_write (mpd->bio);
	  if (status != 0)
	    {
	       if (status != EAGAIN && status != EINTR)
		 {
		   mpd->func = mpd->id = NULL;
		   mpd->state = 0;
		 }
	       return status;
	    }
	  close (mpd->fd);
	  mpd->fd = -1;
	}
    }

  mpd->func = mpd->id = NULL;
  mpd->state = 0;
  return 0;
}

static int
mailbox_pop_get_message (mailbox_t mbox, size_t msgno, message_t *pmsg)
{
  mailbox_pop_data_t mpd;
  bio_t bio;
  int status;
  size_t i;
  void *func = (void *)mailbox_pop_get_message;

  /* sanity */
  if (mbox == NULL || pmsg == NULL || (mpd = mbox->data) == NULL)
    return EINVAL;

  /* see if we already have this message */
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

  /* are we busy in another function ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  /* In the function, but are we busy with an other message/request ? */
  if (mpd->id && mpd->id != *pmsg)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;

  /* Ok men, we're going in */
  switch (mpd->state)
    {
      /* the message */
    case 0:
      {
	message_t msg;
	mailbox_pop_message_t mpm ;

	mpm = calloc (1, sizeof (*mpm));
	if (mpm == NULL)
	  return ENOMEM;

	/* we'll use the bio to store headers */
	mpm->bio = calloc (1, sizeof (*(mpm->bio)));
	if (mpm->bio == NULL)
	  {
	    free (mpm);
	    mpd->func = NULL;
	    return ENOMEM;
	  }

	/* create the message */
	status = message_create (&msg, mpm);
	if (status != 0)
	  {
	    free (mpm->bio);
	    free (mpm);
	    mpd->func = NULL;
	    return status;
	  }

	/* the message */
	mpm->message = msg;
	mpm->num = msgno;
	mpd->mpm = mpm;
	/* back pointer */
	mpm->mpd = mpd;
	/* set the busy request state */
	mpd->id = (void *)msg;

	/*
	 * Get the header.
	 * FIXME: TOP is an optionnal command, if we want to
	 * be compliant we can not count on it to exists.
	 * So we should be prepare when it fails and fall to
	 * a second scheme
	 */
	/*bio->len = snprintf (bio->buffer, POP_BUFSIZ, "TOP %d 0\r\n", msgno);*/
	bio->len = sprintf (bio->buffer, "TOP %d 0\r\n", msgno);
	bio->ptr = bio->buffer;
	mpd->state = 1;
      }
      /* send the TOP */
    case 1:
      {
	status = bio_write (bio);
	if (status != 0)
	  {
	    if (status != EAGAIN && status != EINTR)
	      {
		mpd->func = mpd->id = NULL;
		mpd->state = 0;
		message_destroy (&(mpd->mpm->message), mpd->mpm);
		bio_destroy (&(mpd->mpm->bio));
		free (mpd->mpm);
		mpd->mpm = NULL;
	      }
	    return status;
	  }
	mpd->state = 2;
      }
      /* ack from TOP */
    case 2:
      {
	status = bio_readline (bio);
	if (status != 0)
	  {
	    if (status != EAGAIN && status != EINTR)
	      {
		mpd->func = mpd->id = NULL;
		mpd->state = 0;
		message_destroy (&(mpd->mpm->message), mpd->mpm);
		bio_destroy (&(mpd->mpm->bio));
		free (mpd->mpm);
		mpd->mpm = NULL;
	      }
	    return status;
	  }
	if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	  {
	    mpd->func = mpd->id = NULL;
	    mpd->state = 0;
	    message_destroy (&(mpd->mpm->message), mpd->mpm);
	    bio_destroy (&(mpd->mpm->bio));
	    free (mpd->mpm);
	    mpd->mpm = NULL;
	    return ERANGE;
	  }
      }
      mpd->state = 5;
      /* get the header */
    case 5:
      {
	char *tbuf;
	int nread;
	while (1)
	  {
	    status = bio_readline (bio);
	    if (status != 0)
	      {
		/* recoverable */
		if (status != EAGAIN && status != EINTR)
		  {
		    mpd->func = mpd->id = NULL;
		    mpd->state = 0;
		    message_destroy (&(mpd->mpm->message), mpd->mpm);
		    bio_destroy (&(mpd->mpm->bio));
		    free (mpd->mpm);
		    mpd->mpm = NULL;
		  }
		return status;
	      }

	    /* our ticket out */
	    if (bio->buffer[0] == '\0')
	      break;

	    nread = bio->nl - bio->buffer;

	    tbuf = realloc (mpd->mpm->bio->buffer,
			    mpd->mpm->bio->maxlen + nread);
	    if (tbuf == NULL)
	      {
		mpd->func = mpd->id = NULL;
		mpd->state = 0;
		message_destroy (&(mpd->mpm->message), mpd->mpm);
		bio_destroy (&(mpd->mpm->bio));
		free (mpd->mpm);
		mpd->mpm = NULL;
		return ENOMEM;
	      }
	    else
	      mpd->mpm->bio->buffer = tbuf;
	    memcpy (mpd->mpm->bio->buffer + mpd->mpm->bio->maxlen,
		    bio->buffer, nread);
	    mpd->mpm->bio->maxlen += nread;
	  } /* while () */
      } /* case 5: */
      break;
    default:
      /* error here unknow case */
      fprintf (stderr, "Pop unknown state(get_message)\n");
    } /* switch (state) */

  /* no need to carry a state anymore */
  mpd->func = mpd->id = NULL;
  mpd->state = 0;

  /* create the header */
  {
    header_t header;
    status = header_create (&header, mpd->mpm->bio->buffer,
			    mpd->mpm->bio->maxlen, mpd->mpm);
    bio_destroy (&(mpd->mpm->bio));
    if (status != 0)
      {
	message_destroy (&(mpd->mpm->message), mpd->mpm);
	free (mpd->mpm);
	mpd->mpm = NULL;
	return status;
      }
    message_set_header ((mpd->mpm->message), header, mpd->mpm);
  }

  /* reallocate the working buffer */
  bio_create (&(mpd->mpm->bio), mpd->fd);

  /* create the attribute */
  {
    attribute_t attribute;
    char hdr_status[64];
    header_t header = NULL;
    hdr_status[0] = '\0';
    message_get_header (mpd->mpm->message, &header);
    header_get_value (header, "Status", hdr_status, sizeof (hdr_status), NULL);
    /* create the attribute */
    status = string_to_attribute (hdr_status, &attribute);
    if (status != 0)
      {
	message_destroy (&(mpd->mpm->message), mpd->mpm);
	bio_destroy (&(mpd->mpm->bio));
	free (mpd->mpm);
	mpd->mpm = NULL;
	return status;
      }
    message_set_attribute (mpd->mpm->message, attribute, mpd->mpm);
  }

  /* create the body  */
  {
    stream_t stream;
    body_t body;
    status = body_create (&body, mpd->mpm);
    if (status != 0)
      {
	message_destroy (&(mpd->mpm->message), mpd->mpm);
	bio_destroy (&(mpd->mpm->bio));
	free (mpd->mpm);
	mpd->mpm = NULL;
	return status;
      }
    message_set_body (mpd->mpm->message, body, mpd->mpm);
    status = stream_create (&stream, MU_STREAM_READ, mpd->mpm);
    if (status != 0)
      {
	message_destroy (&(mpd->mpm->message), mpd->mpm);
	bio_destroy (&(mpd->mpm->bio));
	free (mpd->mpm);
	mpd->mpm = NULL;
	return status;
      }
    stream_set_read (stream, mailbox_pop_readstream, mpd->mpm);
    stream_set_fd (stream, mailbox_pop_getfd, mpd->mpm);
    stream_set_flags (stream, mpd->flags, mpd->mpm);
    body_set_size (body, mailbox_pop_body_size, mpd->mpm);
    //body_set_lines (body, mailbox_pop_body_lines, mpd->mpm);
    body_set_stream (body, stream, mpd->mpm);
  }

  /* add it to the list */
  {
    mailbox_pop_message_t *m ;
    m = realloc (mpd->pmessages, (mpd->pmessages_count + 1)*sizeof (*m));
    if (m == NULL)
      {
	message_destroy (&(mpd->mpm->message), mpd->mpm);
	free (mpd->mpm);
	mpd->mpm = NULL;
	return ENOMEM;
      }
    mpd->pmessages = m;
    mpd->pmessages[mpd->pmessages_count] = mpd->mpm;
    mpd->pmessages_count++;
  }

  *pmsg = mpd->mpm->message;

  return 0;
}

static int
mailbox_pop_messages_count (mailbox_t mbox, size_t *pcount)
{
  mailbox_pop_data_t mpd;
  int status;
  void *func = (void *)mailbox_pop_messages_count;
  bio_t bio;

  if (mbox == NULL || (mpd = (mailbox_pop_data_t)mbox->data) == NULL)
    return EINVAL;

  if (mailbox_pop_is_updated (mbox))
    {
      if (pcount)
	*pcount = mpd->messages_count;
      return 0;
    }

  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;
  switch (mpd->state)
    {
    case 0:
      bio->len = sprintf (bio->buffer, "STAT\r\n");
      bio->ptr = bio->buffer;
      mpd->state = 1;
      /* Send the STAT */
    case 1:
      status = bio_write (bio);
      if (status != 0)
	return status;
      mpd->state = 2;
      /* get the ACK */
    case 2:
      status = bio_readline (bio);
      if (status != 0)
	return status;
      break;
    default:
      fprintf (stderr, "unknow state(messages_count)\n");
    }
  mpd->id = mpd->func = NULL;
  mpd->state = 0;

  status = sscanf (bio->buffer, "+OK %d %d", &(mpd->messages_count),
		   &(mpd->size));
  mpd->is_updated = 1;
  if (pcount)
    *pcount = mpd->messages_count;
  if (status == EOF || status != 2)
    return EIO;
  return 0;
}


/* update and scanning*/
static int
mailbox_pop_is_updated (mailbox_t mbox)
{
  mailbox_pop_data_t mpd;
  if ((mpd = (mailbox_pop_data_t)mbox->data) == NULL)
    return 0;
  return mpd->is_updated;
}

static int
mailbox_pop_num_deleted (mailbox_t mbox, size_t *pnum)
{
  mailbox_pop_data_t mpd;
  size_t i, total;
  attribute_t attr;
  if (mbox == NULL ||
      (mpd = (mailbox_pop_data_t) mbox->data) == NULL)
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

/* We just simulated */
static int
mailbox_pop_scan (mailbox_t mbox, size_t msgno, size_t *pcount)
{
  int status;
  size_t i;
  status = mailbox_pop_messages_count (mbox, pcount);
  if (status != 0)
    return status;
  for (i = msgno; i < *pcount; i++)
    if (mailbox_notification (mbox, MU_EVT_MBX_MSG_ADD) != 0)
      break;
  return 0;
}

/* This were we actually sending the DELE command */
static int
mailbox_pop_expunge (mailbox_t mbox)
{
  mailbox_pop_data_t mpd;
  size_t i;
  attribute_t attr;
  bio_t bio;
  int status;
  void *func = (void *)mailbox_pop_expunge;

  if (mbox == NULL ||
      (mpd = (mailbox_pop_data_t) mbox->data) == NULL)
    return EINVAL;

  /* busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  mpd->func = func;
  bio = mpd->bio;

  for (i = (int)mpd->id; i < mpd->pmessages_count; mpd->id = (void *)++i)
    {
      if (message_get_attribute (mpd->pmessages[i]->message, &attr) == 0)
	{
	  /* send DELETE */
	  if (attribute_is_deleted (attr))
	    {
	      switch (mpd->state)
		{
		case 0:
		  bio->len = sprintf (bio->buffer, "DELE %d\r\n",
				      mpd->pmessages[i]->num);
		  bio->ptr = bio->buffer;
		  mpd->state = 1;
		case 1:
		  status = bio_write (bio);
		  if (status != 0)
		    {
		      if (status != EAGAIN && status != EINTR)
			{
			  mpd->func = mpd->id = NULL;
			  mpd->state = 0;
			  fprintf(stderr, "PROBLEM write %d\n", status);
			}
		      return status;
		    }
		  mpd->state = 2;
		case 2:
		  status = bio_readline (bio);
		  if (status != 0)
		    {
		      if (status != EAGAIN && status != EINTR)
			{
			  mpd->func = mpd->id = NULL;
			  mpd->state = 0;
			  fprintf(stderr, "PROBLEM readline %d\n", status);
			}
		      return status;
		    }
		  if (strncasecmp (bio->buffer, "+OK", 3) != 0)
		    {
		      mpd->func = mpd->id = NULL;
		      mpd->state = 0;
			  fprintf(stderr, "PROBLEM strcmp\n");
		      return ERANGE;
		    }
		  mpd->state = 0;
		} /* switch (state) */
	    } /* if attribute_is_deleted() */
	} /* message_get_attribute() */
    } /* for */
  mpd->func = mpd->id = NULL;
  mpd->state = 0;
  /* invalidate */
  mpd->is_updated = 0;

  return 0;
}

/* mailbox size ? */
static int
mailbox_pop_size (mailbox_t mbox, off_t *psize)
{
  mailbox_pop_data_t mpd;
  size_t count;
  int status;

  if (mbox == NULL || (mpd = (mailbox_pop_data_t)mbox->data) == NULL)
    return EINVAL;

  if (mailbox_pop_is_updated (mbox))
    {
      if (psize)
	*psize = mpd->size;
      return 0;
    }
  status = mailbox_pop_messages_count (mbox, &count);
  if (psize)
    *psize = mpd->size;
  return status;
}

static int
mailbox_pop_body_size (body_t body, size_t *psize)
{
  mailbox_pop_message_t mpm;
  if (body == NULL || (mpm = body->owner) == NULL)
    return EINVAL;
  if (psize)
    *psize = mpm->body_size;
  return 0;
}

static int
mailbox_pop_getfd (stream_t stream, int *pfd)
{
  mailbox_pop_message_t mpm;
  if (stream == NULL || (mpm = stream->owner) == NULL)
    return EINVAL;
  if (pfd)
    *pfd = mpm->bio->fd;
  return 0;
}

static int
mailbox_pop_readstream (stream_t is, char *buffer, size_t buflen,
			off_t offset, size_t *pnread)
{
  mailbox_pop_message_t mpm;
  mailbox_pop_data_t mpd;
  size_t nread = 0;
  bio_t bio;
  int status = 0;
  void *func = (void *)mailbox_pop_readstream;

  (void)offset;
  if (is == NULL || (mpm = is->owner) == NULL)
    return EINVAL;

  if (buffer == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = nread;
      return 0;
    }

  bio = mpm->bio;
  mpd = mpm->mpd;

  /* busy ? */
  if (mpd->func && mpd->func != func)
    return EBUSY;

  /* which request */
  if (mpd->id && mpd->id != mpm)
    return EBUSY;

  mpd->func = func;
  mpd->id = mpm;

  switch (mpd->state)
    {
      /* send the RETR command */
    case 0:
      bio->len = sprintf (bio->buffer, "RETR %d\r\n", mpm->num);
      mpd->state = 1;
    case 1:
      status = bio_write (bio);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = mpd->id = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      mpd->state = 2;
      /* RETR ACK */
    case 2:
      status = bio_readline (bio);
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mpd->func = mpd->id = NULL;
	      mpd->state = 0;
	    }
	  return status;
	}
      if (strncasecmp (bio->buffer, "+OK", 3) != 0)
	return EINVAL;
      mpd->state = 3;
      /* skip the header */
    case 3:
      while (!mpm->inbody)
	{
	  status = bio_readline (bio);
	  if (status != 0)
	    {
	      if (status != EAGAIN && status != EINTR)
		{
		  mpd->func = mpd->id = NULL;
		  mpd->state = 0;
		}
	      return status;
	    }
	  if (bio->buffer[0] == '\n')
	    {
	      mpm->inbody = 1;
	      break;
	    }
	}
      /* skip the newline */
      bio_readline (bio);
      /* start taking the header */
      mpd->state = 4;
      bio->current = bio->buffer;
    case 4:
      {
	int nleft, n;
	/* do we need to fill up */
	if (bio->current >= bio->nl)
	  {
	    bio->current = bio->buffer;
	    status = bio_readline (bio);
	    if (status != 0)
	      {
		if (status != EAGAIN && status != EINTR)
		  {
		    mpd->func = mpd->id = NULL;
		    mpd->state = 0;
		  }
	      }
	  }
	n = bio->nl - bio->current;
	nleft = buflen - n;
	/* we got more then requested */
	if (nleft <= 0)
	  {
	    memcpy (buffer, bio->current, buflen);
	    bio->current += buflen;
	    nread = buflen;
	  }
	else
	  {
	    /* drain the buffer */
	    memcpy (buffer, bio->current, n);
	    bio->current += n;
	    nread = n;
	  }
      }
    } /* switch state */

  if (nread == 0)
    {
      mpd->func = mpd->id = NULL;
      mpd->state = 0;
    }
  if (pnread)
    *pnread = nread;
  return 0;
}

static int
bio_create (bio_t *pbio, int fd)
{
  bio_t bio;
  bio = calloc (1, sizeof (*bio));
  if (bio == NULL)
    return ENOMEM;
  bio->maxlen = POP_BUFSIZ + 1;
  bio->current = bio->ptr = bio->buffer = calloc (1, POP_BUFSIZ + 1);
  if (bio->buffer == NULL)
    {
      free (bio);
      return ENOMEM;
    }
  bio->fd = fd;
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
  int nwritten;

  while (bio->len > 0)
    {
      nwritten = write (bio->fd, bio->ptr, bio->len);
      if (nwritten < 0)
	return errno;
      bio->len -= nwritten;
      bio->ptr += nwritten;
    }

  bio->ptr = bio->buffer;
  return 0;
}

static int
bio_read (bio_t bio)
{
  int nread, len;

  len = bio->maxlen - (bio->ptr - bio->buffer) - 1;
  nread = read (bio->fd, bio->ptr, len);
  if (nread < 0)
    return errno;
  else if (nread == 0)
    { /* EOF ???? We got a situation here ??? */
      bio->buffer[0] = '.';
      bio->buffer[1] = '\r';
      bio->buffer[2] = '\n';
      nread = 3;
    }
  bio->ptr[nread] = '\0';
  bio->ptr += nread;
  return 0;
}

/*
 * Responses to certain commands are multi-line.  In these cases, which
 * are clearly indicated below, after sending the first line of the
 * response and a CRLF, any additional lines are sent, each terminated
 * by a CRLF pair.  When all lines of the response have been sent, a
 * final line is sent, consisting of a termination octet (decimal code
 * 046, ".") and a CRLF pair.
 */
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
	  if (status < 0)
	    return status;
	  len = bio->ptr  - bio->buffer;
	  /* a newline */
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

  /*
   * When examining a multi-line response, the client checks
   * to see if the line begins with the termination octet "."(DOT).
   * If so and if octets other than CRLF follow, the first octet
   * of the line (the termination octet) is stripped away.
   */
  if (bio->buffer[0] == '.')
    {
      if (bio->buffer[1] != '\r' && bio->buffer[2] != '\n')
	{
	  memmove (bio->buffer, bio->buffer + 1, bio->ptr - bio->buffer);
	  bio->ptr--;
	  bio->nl--;
	}
      /* If so and if CRLF immediately
       * follows the termination character, then the response from the POP
       * server is ended and the line containing ".CRLF" is not considered
       * part of the multi-line response.
       */
        else if (bio->buffer[1] == '\r' && bio->buffer[2] == '\n')
	{
	  bio->buffer[0] = '\0';
	  bio->nl = bio->ptr = bio->buffer;
	  return 0;
	}
    }

  /* \r\n --> \n\0 */
  if (bio->nl > bio->buffer)
    {
      *(bio->nl - 1) = '\n';
      *(bio->nl) = '\0';
    }

  return 0;
}
