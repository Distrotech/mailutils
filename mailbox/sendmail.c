/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 
   2006 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

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
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/errno.h>

#include <mailer0.h>
#include <registrar0.h>

static struct _mu_record _sendmail_record =
{
  MU_SENDMAIL_PRIO,
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
mu_record_t mu_sendmail_record = &_sendmail_record;

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

static void sendmail_destroy (mu_mailer_t);
static int sendmail_open (mu_mailer_t, int);
static int sendmail_close (mu_mailer_t);
static int sendmail_send_message (mu_mailer_t, mu_message_t, mu_address_t, mu_address_t);

int
_mailer_sendmail_init (mu_mailer_t mailer)
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
    mu_property_t property = NULL;
    mu_mailer_get_property (mailer, &property);
    mu_property_set_value (property, "TYPE", "SENDMAIL", 1);
  }
  return 0;
}

static void
sendmail_destroy(mu_mailer_t mailer)
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
sendmail_open (mu_mailer_t mailer, int flags)
{
  sendmail_t sendmail = mailer->data;
  int status;
  size_t pathlen = 0;
  char *path;

  /* Sanity checks.  */
  if (sendmail == NULL)
    return EINVAL;

  mailer->flags = flags;

  if ((status = mu_url_get_path (mailer->url, NULL, 0, &pathlen)) != 0
      || pathlen == 0)
    return status;

  path = calloc (pathlen + 1, sizeof (char));
  mu_url_get_path (mailer->url, path, pathlen + 1, NULL);

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
sendmail_close (mu_mailer_t mailer)
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
mailer_property_is_set (mu_mailer_t mailer, const char *name)
{
  mu_property_t property = NULL;

  mu_mailer_get_property (mailer, &property);
  return mu_property_is_set (property, name);
}


/* Close FD unless it is part of pipe P */
#define SCLOSE(fd,p) if (p[0]!=fd&&p[1]!=fd) close(fd)

static int
sendmail_send_message (mu_mailer_t mailer, mu_message_t msg, mu_address_t from,
		       mu_address_t to)
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
	argc++;			/* -oi (do not treat '.' as message
				   terminator) */

	if (from)
	  {
	    if ((status = mu_address_aget_email (from, 1, &emailfrom)) != 0)
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
	    status = mu_address_get_email_count (to, &tocount);

	    assert (!status);
	    assert (tocount);

	    argc += tocount;	/* 1 per to address */
	  }

	argc++;		/* -t */

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
	
	if (!to || mailer_property_is_set (mailer, "READ_RECIPIENTS"))
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

	    mu_address_get_count (to, &count);

	    for (; i <= count; i++)
	      {
		char *email = 0;
		if ((status = mu_address_aget_email (to, i, &email)) != 0)
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
		SCLOSE (STDIN_FILENO, tunnel);
		SCLOSE (STDOUT_FILENO, tunnel);
		SCLOSE (STDERR_FILENO, tunnel);
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
	mu_stream_t stream = NULL;
	char buffer[512];
	size_t len = 0;
	int rc;
	size_t offset = 0;
	mu_header_t hdr;
	mu_body_t body;
	int found_nl = 0;
	int exit_status;
	
	mu_message_get_header (msg, &hdr);
	mu_header_get_stream (hdr, &stream);

	MAILER_DEBUG0 (mailer, MU_DEBUG_TRACE, "Sending headers...\n");
	while ((status = mu_stream_readline (stream, buffer, sizeof (buffer),
					  offset, &len)) == 0
	       && len != 0)
	  {
	    if (strncasecmp (buffer, MU_HEADER_FCC,
			     sizeof (MU_HEADER_FCC) - 1))
	      {
		MAILER_DEBUG1 (mailer, MU_DEBUG_PROT,
			       "Header: %s", buffer);
		if (write (sendmail->fd, buffer, len) == -1)
		  {
		    status = errno;
		    
		    MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
				   "write() failed: %s\n", strerror (status));
		    
		    break;
		  }
	      }
	    found_nl = (len == 1 && buffer[0] == '\n');
	      
	    offset += len;
	    sendmail->offset += len;
	  }

	if (!found_nl)
	  {
	    if (write (sendmail->fd, "\n", 1) == -1)
	      {
		status = errno;
		
		MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE,
			       "write() failed: %s\n", strerror (status));
	      }
	  }
	
	mu_message_get_body (msg, &body);
	mu_body_get_stream (body, &stream);

	MAILER_DEBUG0 (mailer, MU_DEBUG_TRACE, "Sending body...\n");
	offset = 0;
	while ((status = mu_stream_read (stream, buffer, sizeof (buffer),
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

	rc = waitpid (sendmail->pid, &exit_status, 0);

	if (rc < 0)
	  {
	    if (errno == ECHILD)
              status = 0;
	    else
              { 
	        status = errno;
	        MAILER_DEBUG2 (mailer, MU_DEBUG_TRACE,
	  		       "waitpid(%d) failed: %s\n",
			       sendmail->pid, strerror (status));
              }
	  }
	else if (WIFEXITED (exit_status))
	  {
	    exit_status = WEXITSTATUS (exit_status);
	    MAILER_DEBUG2 (mailer, MU_DEBUG_TRACE,
			   "%s exited with: %d\n",
			   sendmail->path, exit_status);
	    status = (exit_status == 0) ? 0 : MU_ERR_PROCESS_EXITED;
	  }
	else if (WIFSIGNALED (exit_status))
	  status = MU_ERR_PROCESS_SIGNALED;
	else
	  status = MU_ERR_PROCESS_UNKNOWN_FAILURE;
	
	/* Shouldn't this notification only happen on success? */
	mu_observable_notify (mailer->observable, MU_EVT_MAILER_MESSAGE_SENT);
      }
    default:
      break;
    }

  sendmail->state = (status == 0) ? SENDMAIL_OPEN : SENDMAIL_CLOSED;

  return status;
}

#else
#include <stdio.h>
#include <registrar0.h>
mu_record_t mu_sendmail_record = NULL;
#endif
