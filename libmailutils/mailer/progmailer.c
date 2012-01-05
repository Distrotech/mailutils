/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sysexits.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <mailutils/debug.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/stream.h>
#include <mailutils/header.h>
#include <mailutils/body.h>
#include <mailutils/message.h>
#include <mailutils/progmailer.h>
#include <mailutils/util.h>
#include <mailutils/cstr.h>

struct _mu_progmailer
{
  int fd;
  pid_t pid;
  RETSIGTYPE (*sighandler)();
  char *command;
};

/* Close FD unless it is part of pipe P */
#define SCLOSE(fd,p) if (p[0]!=fd&&p[1]!=fd) close(fd)

int
mu_progmailer_create (struct _mu_progmailer **ppm)
{
  struct _mu_progmailer *pm = malloc (sizeof (*pm));
  if (!pm)
    return ENOMEM;
  pm->fd = -1;
  pm->pid = -1;
  pm->sighandler = SIG_ERR;
  pm->command = NULL;
  *ppm = pm;
  return 0;
}

int
mu_progmailer_set_command (mu_progmailer_t pm, const char *command)
{
  if (!pm)
    return EINVAL;
  free (pm->command);
  if (command)
    {
      pm->command = strdup (command);
      if (!pm->command)
	return EINVAL;
    }
  else
    pm->command = NULL;
  return 0;
}

int
mu_progmailer_sget_command (mu_progmailer_t pm, const char **command)
{
  if (!pm)
    return EINVAL;
  *command = pm->command;
  return 0;
}

void
mu_progmailer_destroy (struct _mu_progmailer **ppm)
{
  if (*ppm)
    {
      free ((*ppm)->command);
      free (*ppm);
      *ppm = NULL;
    }
}

int
mu_progmailer_open (struct _mu_progmailer *pm, char **argv)
{
  int tunnel[2];
  int status = 0;
  int i;

  if (!pm || !argv)
    return EINVAL;
  
  if ((pm->sighandler = signal (SIGCHLD, SIG_DFL)) == SIG_ERR)
    {
      status = errno;
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("setting SIGCHLD failed: %s", mu_strerror (status)));
      return status;
    }
      
  if (pipe (tunnel) == 0)
    {
      pm->fd = tunnel[1];
      pm->pid = fork ();
      if (pm->pid == 0)	/* Child.  */
	{
	  SCLOSE (STDIN_FILENO, tunnel);
	  SCLOSE (STDOUT_FILENO, tunnel);
	  SCLOSE (STDERR_FILENO, tunnel);
	  close (tunnel[1]);
	  dup2 (tunnel[0], STDIN_FILENO);
	  execv (pm->command ? pm->command : argv[0], argv);
	  exit (errno ? EX_CANTCREAT : 0);
	}
      else if (pm->pid == -1)
	{
	  status = errno;
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		    ("fork failed: %s", mu_strerror (status)));
	}
    }
  else
    {
      status = errno;
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("pipe() failed: %s\n", mu_strerror (status)));
    }
  if (mu_debug_level_p (MU_DEBCAT_MAILER, 10))
    {
      mu_debug_log_begin ("exec %s argv:", pm->command);
      for (i = 0; argv[i]; i++)
        mu_debug_log_cont (" %s", argv[i]);
      mu_debug_log_nl ();
    }
  close (tunnel[0]);

  if (status != 0)
    close (pm->fd);
  return status;
}

int
mu_progmailer_send (struct _mu_progmailer *pm, mu_message_t msg)
{
  int status;
  mu_stream_t stream = NULL;
  char buffer[512];
  size_t len = 0;
  int rc;
  mu_header_t hdr;
  mu_body_t body;
  int found_nl = 0;
  int exit_status;
	
  if (!pm || !msg)
    return EINVAL;
  mu_message_get_header (msg, &hdr);
  status = mu_header_get_streamref (hdr, &stream);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("cannot get header stream: %s", mu_strerror (status)));
      return status;
    }
  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE, ("Sending headers..."));
  mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  while ((status = mu_stream_readline (stream, buffer, sizeof (buffer),
				       &len)) == 0
	 && len != 0)
    {
      if (mu_c_strncasecmp (buffer, MU_HEADER_FCC, sizeof (MU_HEADER_FCC) - 1))
	{
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_PROT, ("Header: %s", buffer));
	  if (write (pm->fd, buffer, len) == -1)
	    {
	      status = errno;
	      
	      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
			("write failed: %s", strerror (status)));
	      break;
	    }
	}
      found_nl = (len == 1 && buffer[0] == '\n');
    }

  if (!found_nl)
    {
      if (write (pm->fd, "\n", 1) == -1)
	{
	  status = errno;
		
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		    ("write failed: %s", strerror (status)));
	}
    }
  mu_stream_destroy (&stream);
  
  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE, ("Sending body..."));
  mu_message_get_body (msg, &body);
  status = mu_body_get_streamref (body, &stream);
  if (status)
    {
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("cannot get body stream: %s\n", mu_strerror (status)));
      return status;
    }

  mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  while ((status = mu_stream_read (stream, buffer, sizeof (buffer),
				   &len)) == 0
	 && len != 0)
    {
      if (write (pm->fd, buffer, len) == -1)
	{
	  status = errno;
	  
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		    ("write failed: %s\n", strerror (status)));
	  break;
	}
    }
  mu_body_get_streamref (body, &stream);

  close (pm->fd);

  rc = waitpid (pm->pid, &exit_status, 0);
  if (status == 0)
    {
      if (rc < 0)
	{
	  if (errno == ECHILD)
	    status = 0;
	  else
	    { 
	      status = errno;
	      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
			("waitpid(%lu) failed: %s\n",
			 (unsigned long) pm->pid, strerror (status)));
	    }
	}
      else if (WIFEXITED (exit_status))
	{
	  exit_status = WEXITSTATUS (exit_status);
	  mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_TRACE,
		    ("%s exited with: %d\n",
		     pm->command, exit_status));
	  status = (exit_status == 0) ? 0 : MU_ERR_PROCESS_EXITED;
	}
      else if (WIFSIGNALED (exit_status))
	status = MU_ERR_PROCESS_SIGNALED;
      else
	status = MU_ERR_PROCESS_UNKNOWN_FAILURE;
    }
  pm->pid = -1;
  return status;
}

int
mu_progmailer_close (struct _mu_progmailer *pm)
{
  int status = 0;

  if (!pm)
    return EINVAL;
  
  if (pm->pid > 0)
    {
      kill (SIGTERM, pm->pid);
      pm->pid = -1;
    }

  if (pm->sighandler != SIG_ERR
      && signal (SIGCHLD, pm->sighandler) == SIG_ERR)
    {
      status = errno;
      mu_debug (MU_DEBCAT_MAILER, MU_DEBUG_ERROR,
		("resetting SIGCHLD failed: %s\n", mu_strerror (status)));
    }
  pm->sighandler = SIG_ERR;
  return status;
}
