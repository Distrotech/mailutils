/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

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
#include <mailutils/wordsplit.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/prog_stream.h>
#include <mailutils/util.h>

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



#define REDIRECT_STDIN_P(f) ((f) & MU_STREAM_WRITE)
#define REDIRECT_STDOUT_P(f) ((f) & MU_STREAM_READ)

#ifdef RLIMIT_AS
# define _MU_RLIMIT_AS_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_AS)
#else
# define _MU_RLIMIT_AS_FLAG 0
# define RLIMIT_AS 0
#endif

#ifdef RLIMIT_CPU
# define _MU_RLIMIT_CPU_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_CPU)
#else
# define _MU_RLIMIT_CPU_FLAG 0
# define RLIMIT_CPU 0
#endif

#ifdef RLIMIT_DATA
# define _MU_RLIMIT_DATA_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_DATA)
#else
# define _MU_RLIMIT_DATA_FLAG 0
# define RLIMIT_DATA 0
#endif

#ifdef RLIMIT_FSIZE
# define _MU_RLIMIT_FSIZE_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_FSIZE)
#else
# define _MU_RLIMIT_FSIZE_FLAG 0
# define RLIMIT_FSIZE 0
#endif

#ifdef RLIMIT_NPROC
# define _MU_RLIMIT_NPROC_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_NPROC)
#else
# define _MU_RLIMIT_NPROC_FLAG 0
# define RLIMIT_NPROC 0
#endif

#ifdef RLIMIT_CORE
# define _MU_RLIMIT_CORE_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_CORE)
#else
# define _MU_RLIMIT_CORE_FLAG 0
# define RLIMIT_CORE 0
#endif

#ifdef RLIMIT_MEMLOCK
# define _MU_RLIMIT_MEMLOCK_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_MEMLOCK)
#else
# define _MU_RLIMIT_MEMLOCK_FLAG 0
# define RLIMIT_MEMLOCK 0
#endif

#ifdef RLIMIT_NOFILE
# define _MU_RLIMIT_NOFILE_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_NOFILE)
#else
# define _MU_RLIMIT_NOFILE_FLAG 0
# define RLIMIT_NOFILE 0
#endif

#ifdef RLIMIT_RSS
# define _MU_RLIMIT_RSS_FLAG MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_RSS)
#else
# define _MU_RLIMIT_RSS_FLAG 0
# define RLIMIT_RSS 0
#endif

#ifdef RLIMIT_STACK
# define _MU_RLIMIT_STACK MU_PROG_HINT_LIMIT(MU_PROG_LIMIT_STACK)
#else
# define _MU_RLIMIT_STACK 0
# define RLIMIT_STACK 0
#endif

#define _MU_PROG_AVAILABLE_LIMITS \
  (_MU_RLIMIT_AS_FLAG |		  \
   _MU_RLIMIT_CPU_FLAG |	  \
   _MU_RLIMIT_DATA_FLAG |	  \
   _MU_RLIMIT_FSIZE_FLAG |	  \
   _MU_RLIMIT_NPROC_FLAG |	  \
   _MU_RLIMIT_CORE_FLAG |	  \
   _MU_RLIMIT_MEMLOCK_FLAG |	  \
   _MU_RLIMIT_NOFILE_FLAG |	  \
   _MU_RLIMIT_RSS_FLAG |	  \
   _MU_RLIMIT_STACK)

int _mu_prog_limit_flags = _MU_PROG_AVAILABLE_LIMITS;

int _mu_prog_limit_codes[_MU_PROG_LIMIT_MAX] = {
  RLIMIT_AS,
  RLIMIT_CPU,
  RLIMIT_DATA,
  RLIMIT_FSIZE,
  RLIMIT_NPROC,
  RLIMIT_CORE,
  RLIMIT_MEMLOCK,
  RLIMIT_NOFILE,
  RLIMIT_RSS,
  RLIMIT_STACK
};

static int
start_program_filter (int *p, struct _mu_prog_stream *fs, int flags)
{
  int rightp[2], leftp[2];
  int i;
  int rc = 0;
  
  if (REDIRECT_STDIN_P (flags))
    pipe (leftp);
  if (REDIRECT_STDOUT_P (flags))
    pipe (rightp);
  
  switch (fs->pid = fork ())
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

      if (fs->hint_flags & MU_PROG_HINT_ERRTOOUT)
	dup2 (1, 2);
      
      if (fs->hint_flags & MU_PROG_HINT_WORKDIR)
	{
	  if (chdir (fs->hints.mu_prog_workdir))
	    {
	      mu_error (_("cannot change to %s: %s"),
			fs->hints.mu_prog_workdir, mu_strerror (errno));
	      if (!(fs->hint_flags & MU_PROG_HINT_IGNOREFAIL))
		_exit (127);
	    }
	}

      if (fs->hint_flags & MU_PROG_HINT_UID)
	{
	  if (mu_set_user_privileges (fs->hints.mu_prog_uid,
				      fs->hints.mu_prog_gidv,
				      fs->hints.mu_prog_gidc)
	      && !(fs->hint_flags & MU_PROG_HINT_IGNOREFAIL))
	    _exit (127);
	}
      
      for (i = 0; i < _MU_PROG_LIMIT_MAX; i++)
	{
	  if (MU_PROG_HINT_LIMIT(i) & fs->hint_flags)
	    {
	      struct rlimit rlim;
	      
	      rlim.rlim_cur = rlim.rlim_max = fs->hints.mu_prog_limit[i];
	      if (setrlimit (_mu_prog_limit_codes[i], &rlim))
		{
		  mu_error (_("error setting limit %d to %lu: %s"),
			    i, (unsigned long) rlim.rlim_cur,
			    mu_strerror (errno));
		  if (!(fs->hint_flags & MU_PROG_HINT_IGNOREFAIL))
		    _exit (127);
		}
	    }
	}
      if (MU_PROG_HINT_PRIO & fs->hint_flags)
	{
	  if (setpriority (PRIO_PROCESS, 0, fs->hints.mu_prog_prio))
	    {
	      mu_error (_("error setting priority: %s"),
			mu_strerror (errno));
	      if (!(fs->hint_flags & MU_PROG_HINT_IGNOREFAIL))
		_exit (127);
	    }
	}
       
      /* Close unneded descripitors */
      for (i = mu_getmaxfd (); i > 2; i--)
	close (i);

      /*FIXME: Switch to other uid/gid if desired */
      execvp (fs->progname, fs->argv);
		
      /* Report error via syslog */
      syslog (LOG_ERR|LOG_USER, "can't run %s (ruid=%d, euid=%d): %m",
	      fs->progname, getuid (), geteuid ());
      _exit (127);
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
  int status;
  struct _mu_prog_stream *fs = (struct _mu_prog_stream *) stream;
    
  mu_argcv_free (fs->argc, fs->argv);
  free (fs->progname);
  if (fs->hint_flags & MU_PROG_HINT_WORKDIR)
    free (fs->hints.mu_prog_workdir);
  if (fs->hint_flags & MU_PROG_HINT_INPUT)
    mu_stream_unref (fs->hints.mu_prog_input);
  
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
      mu_stream_copy (fs->out, fs->hints.mu_prog_input, 0, NULL);
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
  
  rc = start_program_filter (pfd, fs, flags);
  if (rc)
    return rc;

  if (REDIRECT_STDOUT_P (flags))
    {
      rc = mu_stdio_stream_create (&fs->in, pfd[0],
				   MU_STREAM_READ|seekable_flag);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
    }
  
  if (REDIRECT_STDIN_P (flags))
    {
      rc = mu_stdio_stream_create (&fs->out, pfd[1],
				   MU_STREAM_WRITE|seekable_flag);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
    }

  _prog_stream_register (fs);
  if (fs->hint_flags & MU_PROG_HINT_INPUT)
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
_prog_ioctl (struct _mu_stream *str, int code, int opcode, void *ptr)
{
  struct _mu_prog_stream *fstr = (struct _mu_prog_stream *) str;
  
  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!ptr)
      	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = ptr;
	  mu_transport_t t[2];

	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      mu_stream_ioctl (fstr->in, MU_IOCTL_TRANSPORT,
			       MU_IOCTL_OP_GET, t);
	      ptrans[0] = t[0];
	      mu_stream_ioctl (fstr->out, MU_IOCTL_TRANSPORT,
			       MU_IOCTL_OP_GET, t);
	      ptrans[1] = t[1];
	      break;
	    case MU_IOCTL_OP_SET:
	      return ENOSYS;
	    default:
	      return EINVAL;
	    }
	}
      break;

    case MU_IOCTL_PROGSTREAM:
      if (!ptr)
      	return EINVAL;
      switch (opcode)
	{
	case MU_IOCTL_PROG_STATUS:
	  *(int*)ptr = fstr->status;
	  break;

	case MU_IOCTL_PROG_PID:
	  *(pid_t*)ptr = fstr->pid;
	  break;

	default:
	  return EINVAL;
	}
      break;
      
    default:
      return ENOSYS;
    }
  return 0;
}

/* NOTE: Steals argv */
static struct _mu_prog_stream *
_prog_stream_create (const char *progname, size_t argc, char **argv,
		     int hint_flags, struct mu_prog_hints *hints, int flags)
{
  struct _mu_prog_stream *fs;

  fs = (struct _mu_prog_stream *) _mu_stream_create (sizeof (*fs), flags);
  if (!fs)
    return NULL;

  fs->progname = strdup (progname);
  if (!fs->progname)
    {
      free (fs);
      return NULL;
    }
  fs->argc = argc;
  fs->argv = argv;
  fs->stream.read = _prog_read;
  fs->stream.write = _prog_write;
  fs->stream.open = _prog_open;
  fs->stream.close = _prog_close;
  fs->stream.ctl = _prog_ioctl;
  fs->stream.flush = _prog_flush;
  fs->stream.done = _prog_done;

  if (!hints)
    fs->hint_flags = 0;
  else
    {
      fs->hint_flags = (hint_flags & _MU_PROG_HINT_MASK) |
	                (hint_flags & _MU_PROG_AVAILABLE_LIMITS);
      if (fs->hint_flags & MU_PROG_HINT_WORKDIR)
	{
	  fs->hints.mu_prog_workdir = strdup (hints->mu_prog_workdir);
	  if (!fs->hints.mu_prog_workdir)
	    {
	      free (fs);
	      return NULL;
	    }
	}
      memcpy (fs->hints.mu_prog_limit, hints->mu_prog_limit,
	      sizeof (fs->hints.mu_prog_limit));
      fs->hints.mu_prog_prio = hints->mu_prog_prio;
      if (fs->hint_flags & MU_PROG_HINT_INPUT)
	{
	  fs->hints.mu_prog_input = hints->mu_prog_input;
	  mu_stream_ref (fs->hints.mu_prog_input);
	}
      if (fs->hint_flags & MU_PROG_HINT_UID)
	{
	  fs->hints.mu_prog_uid = hints->mu_prog_uid;
	  if (fs->hint_flags & MU_PROG_HINT_GID)
	    {
	      fs->hints.mu_prog_gidv = calloc (hints->mu_prog_gidc,
					       sizeof (fs->hints.mu_prog_gidv[0]));
	      if (!fs->hints.mu_prog_gidv)
		{
		  mu_stream_unref ((mu_stream_t) fs);
		  return NULL;
		}
	      memcpy (fs->hints.mu_prog_gidv, hints->mu_prog_gidv,
		      hints->mu_prog_gidc * fs->hints.mu_prog_gidv[0]);
	      fs->hints.mu_prog_gidc = hints->mu_prog_gidc;
	    }
	  else
	    {
	      fs->hints.mu_prog_gidc = 0;
	      fs->hints.mu_prog_gidv = NULL;
	    }
	}
    }
  
  return fs;
}

int
mu_prog_stream_create (mu_stream_t *pstream,
		       const char *progname, size_t argc, char **argv,
		       int hint_flags,
		       struct mu_prog_hints *hints,
		       int flags)
{
  int rc;
  mu_stream_t stream;
  char **xargv;
  size_t i;
    
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (progname == NULL)
    return EINVAL;

  xargv = calloc (argc + 1, sizeof (xargv[0]));
  if (!xargv)
    return ENOMEM;

  for (i = 0; i < argc; i++)
    {
      xargv[i] = strdup (argv[i]);
      if (!xargv[i])
	{
	  mu_argcv_free (i, argv);
	  return ENOMEM;
	}
    }
  stream = (mu_stream_t) _prog_stream_create (progname, argc, xargv,
					      hint_flags, hints, flags);
  if (!stream)
    {
      mu_argcv_free (argc, xargv);
      return ENOMEM;
    }
  
  rc = mu_stream_open (stream);
  if (rc)
    mu_stream_destroy (&stream);
  else
    *pstream = stream;
  return rc;
}

int
mu_command_stream_create (mu_stream_t *pstream, const char *command,
			  int flags)
{
  int rc;
  mu_stream_t stream;
  struct mu_wordsplit ws;
  
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (command == NULL)
    return EINVAL;

  ws.ws_comment = "#";
  if (mu_wordsplit (command, &ws, MU_WRDSF_DEFFLAGS|MU_WRDSF_COMMENT))
    {
      mu_error (_("cannot split line `%s': %s"), command,
		mu_wordsplit_strerror (&ws));
      return errno;
    }

  rc = mu_prog_stream_create (&stream,
			      ws.ws_wordv[0],
			      ws.ws_wordc, ws.ws_wordv,
			      0, NULL, flags);
  if (rc == 0)
    { 
      ws.ws_wordc = 0;
      ws.ws_wordv = NULL;
      *pstream = stream;
    }
  mu_wordsplit_free (&ws);

  return rc;
}
