/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009, 2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>

#include <mailutils/types.h>
#include <mailutils/alloc.h>
#include <mailutils/argcv.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/prog_stream.h>
#include <mailutils/mutil.h>

static mu_list_t prog_stream_list;

static int
_prog_stream_register (struct _mu_prog_stream *stream)
{
  if (!prog_stream_list)
    {
      int rc = mu_list_create (&prog_stream_list);
      if (rc)
	return rc;
    }
  return mu_list_append (prog_stream_list, stream);
}

static void
_prog_stream_unregister (struct _mu_prog_stream *stream)
{
  mu_list_remove (prog_stream_list, stream);
}



#if defined (HAVE_SYSCONF) && defined (_SC_OPEN_MAX)
# define getmaxfd() sysconf (_SC_OPEN_MAX)
#elif defined (HAVE_GETDTABLESIZE)
# define getmaxfd() getdtablesize ()
#else
# define getmaxfd() 64
#endif

#define REDIRECT_STDIN_P(f) ((f) & (MU_STREAM_WRITE|MU_STREAM_RDWR))
#define REDIRECT_STDOUT_P(f) ((f) & (MU_STREAM_READ|MU_STREAM_RDWR))

static int
start_program_filter (pid_t *pid, int *p, int argc, char **argv,
		      char *errfile, int flags)
{
  int rightp[2], leftp[2];
  int i;
  int rc = 0;
  
  if (REDIRECT_STDIN_P (flags))
    pipe (leftp);
  if (REDIRECT_STDOUT_P (flags))
    pipe (rightp);
  
  switch (*pid = fork ())
    {
      /* The child branch.  */
    case 0:
      /* attach the pipes */

      /* Right-end */
      if (REDIRECT_STDOUT_P (flags))
	{
	  if (rightp[1] != 1)
	    {
	      close (1);
	      dup2 (rightp[1], 1);
	    }
	  close (rightp[0]);
	}

      /* Left-end */
      if (REDIRECT_STDIN_P (flags))
	{
	  if (leftp[0] != 0)
	    {
	      close (0);
	      dup2 (leftp[0], 0);
	    }
	  close (leftp[1]);
	}
      
      /* Error output */
      if (errfile)
	{
	  i = open (errfile, O_CREAT|O_WRONLY|O_APPEND, 0644);
	  if (i > 0 && i != 2)
	    {
	      dup2 (i, 2);
	      close (i);
	    }
	}
      /* Close unneded descripitors */
      for (i = getmaxfd (); i > 2; i--)
	close (i);

      /*FIXME: Switch to other uid/gid if desired */
      execvp (argv[0], argv);
		
      /* Report error via syslog */
      syslog (LOG_ERR|LOG_USER, "can't run %s (ruid=%d, euid=%d): %m",
	      argv[0], getuid (), geteuid ());
      exit (127);
      /********************/

      /* Parent branches: */
    case -1:
      /* Fork has failed */
      /* Restore things */
      rc = errno;
      if (REDIRECT_STDOUT_P (flags))
	{
	  close (rightp[0]);
	  close (rightp[1]);
	}
      if (REDIRECT_STDIN_P (flags))
	{
	  close (leftp[0]);
	  close (leftp[1]);
	}
      break;
		
    default:
      if (REDIRECT_STDOUT_P (flags))
	{
	  p[0] = rightp[0];
	  close (rightp[1]);
	}
      else
	p[0] = -1;

      if (REDIRECT_STDIN_P (flags))
	{
	  p[1] = leftp[1];
	  close (leftp[0]);
	}
      else
	p[1] = -1;
    }
  return rc;
}

static void
_prog_wait (pid_t pid, int *pstatus)
{
  if (pid > 0)
    {
      pid_t t;
      do
	t = waitpid (pid, pstatus, 0);
      while (t == -1 && errno == EINTR);
    }
}

static void
_prog_done (mu_stream_t stream)
{
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
  int status;
    
  mu_argcv_free (fs->argc, fs->argv);
  if (fs->in)
    mu_stream_destroy (&fs->in);
  if (fs->out)
    mu_stream_destroy (&fs->out);
  
  _prog_wait (fs->pid, &fs->status);
  fs->pid = -1;
  _prog_wait (fs->writer_pid, &status);
  fs->writer_pid = -1;
  
  _prog_stream_unregister (fs);
}

static int
_prog_close (mu_stream_t stream)
{
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
  int status;
  
  if (!stream)
    return EINVAL;
  
  if (fs->pid <= 0)
    return 0;

  mu_stream_close (fs->out);
  mu_stream_destroy (&fs->out);

  _prog_wait (fs->pid, &fs->status);
  fs->pid = -1;
  _prog_wait (fs->writer_pid, &status);
  fs->writer_pid = -1;
  
  mu_stream_close (fs->in);
  mu_stream_destroy (&fs->in);

  if (WIFEXITED (fs->status))
    {
      if (WEXITSTATUS (fs->status) == 0)
	return 0;
      else if (WEXITSTATUS (fs->status) == 127)
	return MU_ERR_PROCESS_NOEXEC;
      else
	return MU_ERR_PROCESS_EXITED;
    }
  else if (WIFSIGNALED (fs->status))
    return MU_ERR_PROCESS_SIGNALED;
  return MU_ERR_PROCESS_UNKNOWN_FAILURE;
}

static int
feed_input (struct _mu_prog_stream *fs)
{
  pid_t pid;
  size_t size;
  char buffer[128];
  int rc = 0;

  pid = fork ();
  switch (pid)
    {
    default:
      /* Master */
      fs->writer_pid = pid;
      mu_stream_close (fs->out);
      mu_stream_destroy (&fs->out);
      break;
      
    case 0:
      /* Child */
      while (mu_stream_read (fs->input, buffer, sizeof (buffer),
			     &size) == 0
	     && size > 0)
	mu_stream_write (fs->out, buffer, size, NULL);
      mu_stream_close (fs->out);
      exit (0);
      
    case -1:
      rc = errno;
    }

  return rc;
}

static int
_prog_open (mu_stream_t stream)
{
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
  int rc;
  int pfd[2];
  int flags;
  int seekable_flag;
  
  if (!fs || fs->argc == 0)
    return EINVAL;

  if (fs->pid)
    {
      _prog_close (stream);
    }

  mu_stream_get_flags (stream, &flags);
  seekable_flag = (flags & MU_STREAM_SEEK);
  
  rc = start_program_filter (&fs->pid, pfd, fs->argc, fs->argv, NULL, flags);
  if (rc)
    return rc;

  if (REDIRECT_STDOUT_P (flags))
    {
      rc = mu_stdio_stream_create (&fs->in, pfd[0],
				   MU_STREAM_READ|MU_STREAM_AUTOCLOSE|seekable_flag);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
      rc = mu_stream_open (fs->in);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
    }
  
  if (REDIRECT_STDIN_P (flags))
    {
      rc = mu_stdio_stream_create (&fs->out, pfd[1],
				   MU_STREAM_WRITE|MU_STREAM_AUTOCLOSE|seekable_flag);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
      rc = mu_stream_open (fs->out);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
    }

  _prog_stream_register (fs);
  if (fs->input)
    return feed_input (fs);
  return 0;
}

static int
_prog_read (mu_stream_t stream, char *optr, size_t osize, size_t *pnbytes)
{
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
  return mu_stream_read (fs->in, optr, osize, pnbytes);
}

static int
_prog_write (mu_stream_t stream, const char *iptr, size_t isize,
	     size_t *pnbytes)
{
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
  return mu_stream_write (fs->out, iptr, isize, pnbytes);
}

static int
_prog_flush (mu_stream_t stream)
{
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
  mu_stream_flush (fs->in);
  mu_stream_flush (fs->out);
  return 0;
}

static int
_prog_ioctl (struct _mu_stream *str, int code, void *ptr)
{
  struct _mu_prog_stream *fstr = (struct _mu_prog_stream *) str;
  mu_transport_t t[2];
  mu_transport_t *ptrans;
  
  switch (code)
    {
    case MU_IOCTL_GET_TRANSPORT:
      if (!ptr)
      	return EINVAL;
      mu_stream_ioctl (fstr->in, MU_IOCTL_GET_TRANSPORT, t);
      ptrans[0] = t[0];
      mu_stream_ioctl (fstr->out, MU_IOCTL_GET_TRANSPORT, t);
      ptrans[1] = t[1];
      break;

    case MU_IOCTL_GET_STATUS:
      if (!ptr)
      	return EINVAL;
      *(int*)ptr = fstr->status;
      break;

    case MU_IOCTL_GET_PID:
      if (!ptr)
      	return EINVAL;
      *(int*)ptr = fstr->pid;
      break;
      
    default:
      return EINVAL;
    }
  return 0;
}

struct _mu_prog_stream *
_prog_stream_create (const char *progname, int flags)
{
  struct _mu_prog_stream *fs;
  
  fs = (struct _mu_prog_stream *) _mu_stream_create (sizeof (*fs), flags);
  if (!fs)
    return NULL;

  if (mu_argcv_get (progname, "", "#", &fs->argc, &fs->argv))
    {
      mu_argcv_free (fs->argc, fs->argv);
      free (fs);
      return NULL;
    }

  fs->stream.read = _prog_read;
  fs->stream.write = _prog_write;
  fs->stream.open = _prog_open;
  fs->stream.close = _prog_close;
  fs->stream.ctl = _prog_ioctl;
  fs->stream.flush = _prog_flush;
  fs->stream.done = _prog_done;

  return fs;
}

int
mu_prog_stream_create (mu_stream_t *pstream, const char *progname, int flags)
{
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (progname == NULL)
    return EINVAL;

  if ((*pstream = (mu_stream_t) _prog_stream_create (progname, flags)) == NULL)
    return ENOMEM;
  return 0;
}

int
mu_filter_prog_stream_create (mu_stream_t *pstream, const char *progname,
			      mu_stream_t input)
{
  struct _mu_prog_stream *fs;

  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (progname == NULL)
    return EINVAL;

  fs = _prog_stream_create (progname, MU_STREAM_RDWR);
  if (!fs)
    return ENOMEM;
  mu_stream_ref (input);
  fs->input = input;
  *pstream = (mu_stream_t) fs;
  return 0;
}

