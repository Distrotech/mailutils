/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifdef ENABLE_SENDMAIL

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <mailutils/address.h>
#include <mailutils/debug.h>
#include <mailutils/message.h>
#include <mailutils/observer.h>
#include <mailutils/property.h>
#include <mailutils/stream.h>
#include <mailutils/url.h>

#include <mailer0.h>
#include <registrar0.h>

static struct _record _sendmail_record =
{
  MU_SENDMAIL_SCHEME,
  _url_sendmail_init,    /* url init.  */
  NULL,                  /* Mailbox entry.  */
  _mailer_sendmail_init, /* Mailer entry.  */
  NULL, /* Folder entry.  */
  NULL, /* No need for a back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
/* We export, url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
record_t sendmail_record = &_sendmail_record;

struct _sendmail
{
  int dsn;
  char *path;
  pid_t pid;
  off_t offset;
  int fd;
  enum sendmail_state { SENDMAIL_CLOSED, SENDMAIL_OPEN, SENDMAIL_SEND } state;
};

typedef struct _sendmail * sendmail_t;

static void sendmail_destroy (mailer_t);
static int sendmail_open (mailer_t, int);
static int sendmail_close (mailer_t);
static int sendmail_send_message (mailer_t, message_t, address_t, address_t);

int
_mailer_sendmail_init (mailer_t mailer)
{
  sendmail_t sendmail;

  /* Allocate memory specific to sendmail mailer.  */
  sendmail = mailer->data = calloc (1, sizeof (*sendmail));
  if (mailer->data == NULL)
    return ENOMEM;
  sendmail->state = SENDMAIL_CLOSED;
  mailer->_destroy = sendmail_destroy;
  mailer->_open = sendmail_open;
  mailer->_close = sendmail_close;
  mailer->_send_message = sendmail_send_message;

  /* Set our properties.  */
  {
    property_t property = NULL;
    mailer_get_property (mailer, &property);
    property_set_value (property, "TYPE", "SENDMAIL", 1);
  }
  return 0;
}

static void
sendmail_destroy(mailer_t mailer)
{
  sendmail_t sendmail = mailer->data;
  if (sendmail)
    {
      if (sendmail->path)
	free (sendmail->path);
      free (sendmail);
      mailer->data = NULL;
    }
}

static int
sendmail_open (mailer_t mailer, int flags)
{
  sendmail_t sendmail = mailer->data;
  int status;
  size_t pathlen = 0;
  char *path;

  /* Sanity checks.  */
  if (sendmail == NULL)
    return EINVAL;

  mailer->flags = flags;

  if ((status = url_get_path (mailer->url, NULL, 0, &pathlen)) != 0
      || pathlen == 0)
    return status;

  path = calloc (pathlen + 1, sizeof (char));
  url_get_path (mailer->url, path, pathlen + 1, NULL);

  if (access (path, X_OK) == -1)
    {
      free (path);
      return errno;
    }
  sendmail->path = path;
  sendmail->state = SENDMAIL_OPEN;
  MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE, "sendmail (%s)\n", sendmail->path);
  return 0;
}

static int
sendmail_close (mailer_t mailer)
{
  sendmail_t sendmail = mailer->data;

  /* Sanity checks.  */
  if (sendmail == NULL)
    return EINVAL;

  if(sendmail->path)
    free(sendmail->path);

  sendmail->path = NULL;
  sendmail->state = SENDMAIL_CLOSED;

  return 0;
}

static int
sendmail_send_message (mailer_t mailer, message_t msg, address_t from,
		       address_t to)
{
  sendmail_t sendmail = mailer->data;
  int status = 0;

  if (sendmail == NULL || msg == NULL)
    return EINVAL;

  switch (sendmail->state)
    {
    case SENDMAIL_CLOSED:
      return EINVAL;
    case SENDMAIL_OPEN:
      {
	int tunnel[2];
	int argc = 0;
	char **argvec = NULL;
	size_t tocount = 0;
	char *emailfrom = NULL;

	/* Count the length of the arg vec: */

	argc++;			/* terminating NULL */
	argc++;			/* sendmail */
	argc++;			/* -oi (do not treat '.' as message terminator) */

	if (from)
	  {
	    if ((status = address_aget_email (from, 1, &emailfrom)) != 0)
	      goto OPEN_STATE_CLEANUP;

	    if (!emailfrom)
	      {
		/* the address wasn't fully qualified, choke (for now) */
		status = EINVAL;

		MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
			       "envelope from (%s) not fully qualifed\n",
			       emailfrom);

		goto OPEN_STATE_CLEANUP;
	      }

	    argc += 2;		/* -f from */
	  }
	if (to)
	  {
	    status = address_get_email_count (to, &tocount);

	    assert (!status);
	    assert (tocount);

	    argc += tocount;	/* 1 per to address */
	  }
	else
	  {
	    argc++;		/* -t */
	  }

	/* Allocate arg vec: */
	if ((argvec = calloc (argc, sizeof (*argvec))) == 0)
	  {
	    status = ENOMEM;
	    goto OPEN_STATE_CLEANUP;
	  }

	argc = 0;

	if ((argvec[argc++] = strdup (sendmail->path)) == 0)
	  {
	    status = ENOMEM;
	    goto OPEN_STATE_CLEANUP;
	  }

	if ((argvec[argc++] = strdup ("-oi")) == 0)
	  {
	    status = ENOMEM;
	    goto OPEN_STATE_CLEANUP;
	  }
	if (from)
	  {
	    if ((argvec[argc++] = strdup ("-f")) == 0)
	      {
		status = ENOMEM;
		goto OPEN_STATE_CLEANUP;
	      }
	    argvec[argc++] = emailfrom;
	  }
	if (!to)
	  {
	    if ((argvec[argc++] = strdup ("-t")) == 0)
	      {
		status = ENOMEM;
		goto OPEN_STATE_CLEANUP;
	      }
	  }
	else
	  {
	    int i = 1;
	    size_t count = 0;

	    address_get_count (to, &count);

	    for (; i <= count; i++)
	      {
		char *email = 0;
		if ((status = address_aget_email (to, i, &email)) != 0)
		  goto OPEN_STATE_CLEANUP;
		if (!email)
		  {
		    /* the address wasn't fully qualified, choke (for now) */
		    status = EINVAL;

		    MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
				   "envelope to (%s) not fully qualifed\n",
				   email);

		    goto OPEN_STATE_CLEANUP;
		  }
		argvec[argc++] = email;
	      }
	  }

	assert (argvec[argc] == NULL);

	if (pipe (tunnel) == 0)
	  {
	    sendmail->fd = tunnel[1];
	    sendmail->pid = vfork ();
	    if (sendmail->pid == 0)	/* Child.  */
	      {
		close (STDIN_FILENO);
		close (STDOUT_FILENO);
		close (STDERR_FILENO);
		close (tunnel[1]);
		dup2 (tunnel[0], STDIN_FILENO);
		execv (sendmail->path, argvec);
		exit (errno);
	      }
	    else if (sendmail->pid == -1)
	      {
		status = errno;

		MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
			       "vfork() failed: %s\n", strerror (status));
	      }
	  }
	else
	  {
	    status = errno;
	    MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
			   "pipe() failed: %s\n", strerror (status));
	  }

      OPEN_STATE_CLEANUP:
	MAILER_DEBUG0 (mailer, MU_DEBUG_TRACE, "exec argv:");
	for (argc = 0; argvec && argvec[argc]; argc++)
	  {
	    MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE, " %s", argvec[argc]);
	    free (argvec[argc]);
	  }
	MAILER_DEBUG0 (mailer, MU_DEBUG_TRACE, "\n");
	free (argvec);
	close (tunnel[0]);

	if (status != 0)
	  {
	    close (sendmail->fd);
	    break;
	  }
	sendmail->state = SENDMAIL_SEND;
      }

    case SENDMAIL_SEND:
      {
	stream_t stream = NULL;
	char buffer[512];
	size_t len = 0;
	int rc;
	size_t offset = 0;
	
	message_get_stream (msg, &stream);
	while ((status = stream_read (stream, buffer, sizeof (buffer),
				      offset, &len)) == 0
	       && len != 0)
	  {
	    if (write (sendmail->fd, buffer, len) == -1)
	      {
		status = errno;

		MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
			       "write() failed: %s\n", strerror (status));

		break;
	      }
	    offset += len;
	    sendmail->offset += len;
	  }
	if (status == EAGAIN)
	  return status;

	close (sendmail->fd);

	rc = waitpid (sendmail->pid, &status, 0);

	if (rc < 0)
	  {
	    status = errno;
	    MAILER_DEBUG2 (mailer, MU_DEBUG_TRACE,
			   "waitpid(%d) failed: %s\n",
			   sendmail->pid, strerror (status));
	  }
	else if (WIFEXITED (status))
	  {
	    status = WEXITSTATUS (status);
	    MAILER_DEBUG2 (mailer, MU_DEBUG_TRACE,
			   "%s exited with: %s\n",
			   sendmail->path, strerror (status));
	  }
	/* Shouldn't this notification only happen on success? */
	observable_notify (mailer->observable, MU_EVT_MAILER_MESSAGE_SENT);
      }
    default:
      break;
    }

  sendmail->state = SENDMAIL_OPEN;

  return status;
}

#else
#include <stdio.h>
#include <registrar0.h>
record_t sendmail_record = NULL;
#endif
