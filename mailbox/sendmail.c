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
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/stream.h>
#include <mailer0.h>
#include <registrar0.h>
#include <misc.h>

static int sendmail_init (mailer_t);

static struct mailer_entry _sendmail_entry =
{
  url_sendmail_init, sendmail_init
};
static struct _record _sendmail_record =
{
  MU_SENDMAIL_SCHEME,
  NULL, /* Mailbox entry.  */
  &_sendmail_entry, /* Mailer entry.  */
  NULL, /* Folder entry.  */
  0, /* Not malloc()ed.  */
  NULL, /* No need for an owner.  */
  NULL, /* is_scheme method.  */
  NULL, /* get_mailbox method.  */
  NULL, /* get_mailer method.  */
  NULL /* get_folder method.  */
};

/* We export two functions: url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
record_t sendmail_record = &_sendmail_record;
mailer_entry_t sendmail_entry = &_sendmail_entry;

struct _sendmail
{
  int dsn;
  char *path;
  pid_t pid;
  off_t offset;
  int fd;
  enum sendmail_state { SENDMAIL_NO_STATE, SENDMAIL_SEND } state;
};

typedef struct _sendmail * sendmail_t;

static void sendmail_destroy (mailer_t);
static int sendmail_open (mailer_t, int);
static int sendmail_close (mailer_t);
static int sendmail_send_message (mailer_t, message_t);

int
sendmail_init (mailer_t mailer)
{
  sendmail_t sendmail;

  /* Allocate memory specific to sendmail mailer.  */
  sendmail = mailer->data = calloc (1, sizeof (*sendmail));
  if (mailer->data == NULL)
    return ENOMEM;
  sendmail->state = SENDMAIL_NO_STATE;
  mailer->_init = sendmail_init;
  mailer->_destroy = sendmail_destroy;

  mailer->_open = sendmail_open;
  mailer->_close = sendmail_close;
  mailer->_send_message = sendmail_send_message;

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

  mailer->flags = flags | MU_STREAM_SENDMAIL;

  /* Fetch the mailer server name and the port in the url_t.  */
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
  MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE, "sendmail (%s)\n", sendmail->path);
  return 0;
}

static int
sendmail_close (mailer_t mailer)
{
  (void)mailer;
  return 0;
}

static int
sendmail_send_message (mailer_t mailer, message_t msg)
{
  sendmail_t sendmail = mailer->data;
  int status = 0;

  if (sendmail == NULL || msg == NULL)
    return EINVAL;

  switch (sendmail->state)
    {
    case SENDMAIL_NO_STATE:
      {
	int tunnel[2];
	int argc = 3;
	char **argvec = NULL;

	argvec = realloc (argvec, argc * (sizeof (*argvec)));
	argvec[0] = strdup (sendmail->path);
	/* do not treat '.' as message terminator*/
	argvec[1] = strdup ("-oi");
	argvec[2] = strdup ("-t");

	argc++;
	argvec = realloc (argvec, argc * (sizeof (*argvec)));
	argvec[argc - 1] = NULL;

	if (pipe (tunnel) == 0)
	  {
	    sendmail->fd = tunnel [1];
	    sendmail->pid = fork ();
	    if (sendmail->pid == 0) /* Child.  */
	      {
		close (STDIN_FILENO);
		close (STDOUT_FILENO);
		close (STDERR_FILENO);
		close (tunnel[1]);
		dup2 (tunnel[0], STDIN_FILENO);
		execv (sendmail->path, argvec);
		exit (1);
	      }
	    else if (sendmail->pid == -1)
	      status = errno;
	  }
	else
	  status = errno;
	for (argc = 0; argvec[argc]; argc++)
	  {
	    MAILER_DEBUG1 (mailer, MU_DEBUG_TRACE, "%s ", argvec[argc]);
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

    case SENDMAIL_SEND: /* Parent.  */
      {
	stream_t stream = NULL;
	char buffer[512];
	size_t len = 0;
	int rc;

	message_get_stream (msg, &stream);
	while ((status = stream_read (stream, buffer, sizeof (buffer),
				      sendmail->offset, &len)) == 0
	       && len != 0)
	  {
	    if (write (sendmail->fd, buffer, len) == -1)
	      {
		status = errno;
		break;
	      }
	    sendmail->offset += len;
	  }
	if (status == EAGAIN)
	  return status;
	close (sendmail->fd);
	rc = waitpid(sendmail->pid, &status, 0);
        if (rc < 0)
	  status = errno;
	else if (WIFEXITED(status))
	  status = WEXITSTATUS(status);
      }
    default:
      break;
    }

  sendmail->state = SENDMAIL_NO_STATE;
  return status;
}
