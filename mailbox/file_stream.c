/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>

#include <mailutils/stream.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/argcv.h>
#include <mailutils/nls.h>
#include <mailutils/list.h>

struct _file_stream
{
  FILE *file;
  int offset;

  char *filename;
  stream_t cache;
};

static void
_file_destroy (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);

  if (fs->filename)
    free (fs->filename);

  if (fs->cache)
    stream_destroy (&fs->cache, stream_get_owner (fs->cache));
  free (fs);
}

static int
_file_read (stream_t stream, char *optr, size_t osize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  size_t n;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
	return errno;
      fs->offset = offset;
    }

  n = fread (optr, sizeof(char), osize, fs->file);
  if (n == 0)
    {
      if (ferror(fs->file))
	err = errno;
    }
  else
    fs->offset += n;

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_file_readline (stream_t stream, char *optr, size_t osize,
		off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  size_t n = 0;
  int err = 0;

  if (!fs->file)
    {
      optr[0] = 0;
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
	return errno;
      fs->offset = offset;
    }

  if (fgets (optr, osize, fs->file) != NULL)
    {
      char *tmp = optr;
      while (*tmp) tmp++; /* strlen(optr) */
      n = tmp - optr;
      /* !!!!! WTF ??? */
      if (n == 0)
	n++;
      else
	fs->offset += n;
    }
  else
    {
      if (ferror (fs->file))
	err = errno;
    }

  optr[n] = 0;
  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_file_write (stream_t stream, const char *iptr, size_t isize,
	    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  size_t n;
  int err = 0;

  if (!fs->file)
    {
      if (nbytes)
	*nbytes = 0;
      return 0;
    }

  if (fs->offset != offset)
    {
      if (fseek (fs->file, offset, SEEK_SET) != 0)
	return errno;
      fs->offset = offset;
    }

  n = fwrite (iptr, sizeof(char), isize, fs->file);
  if (n != isize)
    {
      if (feof (fs->file) == 0)
	err = EIO;
      clearerr(fs->file);
      n = 0;
    }
  else
    fs->offset += n;

  if (nbytes)
    *nbytes = n;
  return err;
}

static int
_stdin_file_read (stream_t stream, char *optr, size_t osize,
		  off_t offset, size_t *pnbytes)
{
  int status = 0;
  size_t nbytes;
  struct _file_stream *fs = stream_get_owner (stream);
  int fs_offset = fs->offset;

  if (offset < fs_offset)
    return stream_read (fs->cache, optr, osize, offset, pnbytes);
  else if (offset > fs_offset)
    {
      int status = 0;
      size_t n, left = offset - fs_offset + 1;
      char *buf = malloc (left);
      if (!buf)
	return ENOMEM;
      while (left > 0
	     && (status = stream_read (stream, buf, left, fs_offset, &n)) == 0
	     && n > 0)
	{
	  size_t k;
	  status = stream_write (fs->cache, buf, n, fs_offset, &k);
	  if (status)
	    break;
	  if (k != n)
	    {
	      status = EIO;
	      break;
	    }
	  
	  fs_offset += n;
	  left -= n;
	}
      free (buf);
      if (status)
	return status;
    }
  
  if (feof (fs->file))
    nbytes = 0;
  else
    {
      status = _file_read (stream, optr, osize, fs_offset, &nbytes);
      if (status == 0 && nbytes)
	{
	  size_t k;

	  status = stream_write (fs->cache, optr, nbytes, fs_offset, &k);
	  if (status)
	    return status;
	  if (k != nbytes)
	    return EIO;
	}
    }
  if (pnbytes)
    *pnbytes = nbytes;
  return status;
}

static int
_stdin_file_readline (stream_t stream, char *optr, size_t osize,
		      off_t offset, size_t *pnbytes)
{
  int status;
  size_t nbytes;
  struct _file_stream *fs = stream_get_owner (stream);
  int fs_offset = fs->offset;
  
  if (offset < fs->offset)
    return stream_readline (fs->cache, optr, osize, offset, pnbytes);
  else if (offset > fs->offset)
    return ESPIPE;

  fs_offset = fs->offset;
  status = _file_readline (stream, optr, osize, fs_offset, &nbytes);
  if (status == 0)
    {
      size_t k;

      status = stream_write (fs->cache, optr, nbytes, fs_offset, &k);
      if (status)
	return status;
      if (k != nbytes)
	return EIO;
    }
  if (pnbytes)
    *pnbytes = nbytes;
  return status;
}

static int
_stdout_file_write (stream_t stream, const char *iptr, size_t isize,
		    off_t offset, size_t *nbytes)
{
  struct _file_stream *fs = stream_get_owner (stream);
  return _file_write (stream, iptr, isize, fs->offset, nbytes);
}

static int
_file_truncate (stream_t stream, off_t len)
{
  struct _file_stream *fs = stream_get_owner (stream);
  if (fs->file && ftruncate (fileno(fs->file), len) != 0)
    return errno;
  return 0;
}

static int
_file_size (stream_t stream, off_t *psize)
{
  struct _file_stream *fs = stream_get_owner (stream);
  struct stat stbuf;
  if (!fs->file)
    {
      if (psize)
	*psize = 0;
      return 0;
    }
  fflush (fs->file);
  if (fstat(fileno(fs->file), &stbuf) == -1)
    return errno;
  if (psize)
    *psize = stbuf.st_size;
  return 0;
}

static int
_file_flush (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);
  if (fs->file)
    return fflush (fs->file);
  return 0;
}

static int
_file_get_fd (stream_t stream, int *pfd, int *pfd2)
{
  struct _file_stream *fs = stream_get_owner (stream);
  int status = 0;

  if (pfd2)
    return ENOSYS;
  
  if (pfd)
    {
      if (fs->file)
	*pfd = fileno (fs->file);
      else
	status = EINVAL;
    }
  return status;
}

static int
_file_close (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);
  int err = 0;

  if (!stream)
    return EINVAL;

  if (fs->file)
    {
      int flags = 0;

      stream_get_flags (stream, &flags);

      if ((flags & MU_STREAM_NO_CLOSE) == 0)
	{
	  if (fclose (fs->file) != 0)
	    err = errno;
	}
      
      fs->file = NULL;
    }
  return err;
}

static int
_file_open (stream_t stream)
{
  struct _file_stream *fs = stream_get_owner (stream);
  int flg;
  int fd;
  const char *mode;
  char* filename = 0;
  int flags = 0;

  assert(fs);

  filename = fs->filename;

  assert(filename);

  if (fs->file)
    {
      fclose (fs->file);
      fs->file = NULL;
    }

  stream_get_flags(stream, &flags);

  /* Map the flags to the system equivalent.  */
  if (flags & MU_STREAM_WRITE && flags & MU_STREAM_READ)
    return EINVAL;
  else if (flags & MU_STREAM_WRITE)
    flg = O_WRONLY;
  else if (flags & MU_STREAM_RDWR)
    flg = O_RDWR;
  else /* default */
    flg = O_RDONLY;

  /* Local folders should not block it is local disk ???
     We simply ignore the O_NONBLOCK flag
     But take care of the APPEND.  */
  if (flags & MU_STREAM_APPEND)
    flg |= O_APPEND;

  /* Handle CREAT with care, not to follow symlinks.  */
  if (flags & MU_STREAM_CREAT)
    {
      /* First see if the file already exists.  */
      fd = open(filename, flg);
      if (fd == -1)
	{
	  /* Oops bail out.  */
	  if (errno != ENOENT)
	    return errno;
	  /* Race condition here when creating the file ??.  */
	  fd = open(filename, flg|O_CREAT|O_EXCL, 0600);
	  if (fd < 0)
	    return errno;
	}
    }
  else
    {
      fd = open (filename, flg);
      if (fd < 0)
        return errno;
    }

  /* We have to make sure that We did not open
     a symlink. From Casper D. in bugtraq.  */
  if ((flg & MU_STREAM_CREAT) ||
      (flg & MU_STREAM_RDWR) ||
      (flg & MU_STREAM_WRITE))
    {
      struct stat fdbuf, filebuf;

      /* The next two stats should never fail.  */
      if (fstat(fd, &fdbuf) == -1)
	return errno;
      if (lstat(filename, &filebuf) == -1)
	return errno;

      /* Now check that: file and fd reference the same file,
	 file only has one link, file is plain file.  */
      if (!(flags & MU_STREAM_ALLOW_LINKS)
	  && (fdbuf.st_dev != filebuf.st_dev
	      || fdbuf.st_ino != filebuf.st_ino
	      || fdbuf.st_nlink != 1
	      || filebuf.st_nlink != 1
	      || (fdbuf.st_mode & S_IFMT) != S_IFREG))
	{
	  mu_error (_("%s must be a plain file with one link\n"), filename);
	  close (fd);
	  return EINVAL;
	}
    }
  /* We use FILE * object.  */
  if (flags & MU_STREAM_APPEND)
    mode = "a";
  else if (flags & MU_STREAM_RDWR)
    mode = "r+b";
  else if (flags & MU_STREAM_WRITE)
    mode = "wb";
  else /* Default readonly.  */
    mode = "rb";

  fs->file = fdopen (fd, mode);
  if (fs->file == NULL)
    return errno;

  return 0;
}

int
_file_strerror (stream_t unused, char **pstr)
{
  *pstr = strerror (errno);
  return 0;
}

int
file_stream_create (stream_t *stream, const char* filename, int flags)
{
  struct _file_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  fs = calloc (1, sizeof (struct _file_stream));
  if (fs == NULL)
    return ENOMEM;

  if ((fs->filename = strdup(filename)) == NULL)
  {
    free (fs);
    return ENOMEM;
  }

  ret = stream_create (stream, flags|MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      free (fs);
      free (fs->filename);
      return ret;
    }

  stream_set_open (*stream, _file_open, fs);
  stream_set_close (*stream, _file_close, fs);
  stream_set_fd (*stream, _file_get_fd, fs);
  stream_set_read (*stream, _file_read, fs);
  stream_set_readline (*stream, _file_readline, fs);
  stream_set_write (*stream, _file_write, fs);
  stream_set_truncate (*stream, _file_truncate, fs);
  stream_set_size (*stream, _file_size, fs);
  stream_set_flush (*stream, _file_flush, fs);
  stream_set_destroy (*stream, _file_destroy, fs);
  stream_set_strerror (*stream, _file_strerror, fs);
  
  return 0;
}

int
stdio_stream_create (stream_t *stream, FILE *file, int flags)
{
  struct _file_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  if (file == NULL)
    return EINVAL;

  fs = calloc (1, sizeof (struct _file_stream));
  if (fs == NULL)
    return ENOMEM;

  fs->file = file;

  ret = stream_create (stream, flags|MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      free (fs);
      return ret;
    }

  /* Check if we need to enable caching */

  if ((flags & MU_STREAM_SEEKABLE) && lseek (fileno (file), 0, 0))
    {
      if ((ret = memory_stream_create (&fs->cache, 0, MU_STREAM_RDWR))
	  || (ret = stream_open (fs->cache)))
	{
	  stream_destroy (stream, fs);
	  free (fs);
	  return ret;
	}
      stream_set_read (*stream, _stdin_file_read, fs);
      stream_set_readline (*stream, _stdin_file_readline, fs);
      stream_set_write (*stream, _stdout_file_write, fs);
    }
  else
    {
      stream_set_read (*stream, _file_read, fs);
      stream_set_readline (*stream, _file_readline, fs);
      stream_set_write (*stream, _file_write, fs);
    }
  
  /* We don't need to open the FILE, just return success. */

  stream_set_open (*stream, NULL, fs);
  stream_set_close (*stream, _file_close, fs);
  stream_set_fd (*stream, _file_get_fd, fs);
  stream_set_flush (*stream, _file_flush, fs);
  stream_set_destroy (*stream, _file_destroy, fs);

  return 0;
}


struct _prog_stream
{
  pid_t pid;
  int status;
  pid_t writer_pid;
  size_t argc;
  char **argv;
  stream_t in, out;

  stream_t input;
};

static list_t prog_stream_list;

static int
_prog_stream_register (struct _prog_stream *stream)
{
  if (!prog_stream_list)
    {
      int rc = list_create (&prog_stream_list);
      if (rc)
	return rc;
    }
  return list_append (prog_stream_list, stream);
}

static void
_prog_stream_unregister (struct _prog_stream *stream)
{
  list_remove (prog_stream_list, stream);
}

static int
_prog_waitpid (void *item, void *data ARG_UNUSED)
{
  struct _prog_stream *str = item;
  int status;
  if (str->pid > 0)
    {
      if (waitpid (str->pid, &str->status, WNOHANG) == str->pid)
	str->pid = -1;
    }
  if (str->writer_pid > 0)
    waitpid (str->writer_pid, &status, WNOHANG);
  return 0;
}

static void
_prog_stream_wait (struct _prog_stream *stream)
{
  list_do (prog_stream_list, _prog_waitpid, NULL);
  if (stream->pid > 0)
    {
      kill (stream->pid, SIGTERM);
      kill (stream->pid, SIGKILL);
      kill (stream->writer_pid, SIGKILL);
      list_do (prog_stream_list, _prog_waitpid, NULL);
    }
}

#if defined (HAVE_SYSCONF) && defined (_SC_OPEN_MAX)
# define getmaxfd() sysconf (_SC_OPEN_MAX)
#elif defined (HAVE_GETDTABLESIZE)
# define getmaxfd() getdtablesize ()
#else
# define getmaxfd() 64
#endif
     
static int
start_program_filter (pid_t *pid, int *p, int argc, char **argv, char *errfile)
{
  int rightp[2], leftp[2];
  int i;
  int rc = 0;
  
  pipe (leftp);
  pipe (rightp);
  
  switch (*pid = fork ())
    {
      /* The child branch.  */
    case 0:
      /* attach the pipes */

      /* Right-end */
      if (rightp[1] != 1)
	{
	  close (1);
	  dup2 (rightp[1], 1);
	}
      close (rightp[0]); 

      /* Left-end */
      if (leftp[0] != 0)
	{
	  close (0);
	  dup2 (leftp[0], 0);
	}
      close (leftp[1]);

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

      syslog (LOG_ERR|LOG_USER, "run %s %s",
	      argv[0], argv[1]);
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
      close (rightp[0]);
      close (rightp[1]);
      close (leftp[0]);
      close (leftp[1]);
      break;
		
    default:
      p[0] = rightp[0];
      close (rightp[1]);
		
      p[1] = leftp[1];
      close (leftp[0]);
    }
  return rc;
}

static void
_prog_destroy (stream_t stream)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  argcv_free (fs->argc, fs->argv);
  if (fs->in)
    stream_destroy (&fs->in, stream_get_owner (fs->in));
  if (fs->out)
    stream_destroy (&fs->out, stream_get_owner (fs->out));
  _prog_stream_unregister (fs);
}

static int
_prog_close (stream_t stream)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  
  if (!stream)
    return EINVAL;
  
  if (fs->pid <= 0)
    return 0;

  _prog_stream_wait (fs);
  
  stream_close (fs->in);
  stream_destroy (&fs->in, stream_get_owner (fs->in));

  stream_close (fs->out);
  stream_destroy (&fs->out, stream_get_owner (fs->out));

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
feed_input (struct _prog_stream *fs)
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
      stream_close (fs->out);
      stream_destroy (&fs->out, stream_get_owner (fs->out));
      break;
      
    case 0:
      /* Child */
      while (stream_sequential_read (fs->input, buffer, sizeof (buffer),
				     &size) == 0
	     && size > 0)
	stream_sequential_write (fs->out, buffer, size);
      stream_close (fs->out);
      exit (0);
      
    case -1:
      rc = errno;
    }

  return rc;
}
  
static int
_prog_open (stream_t stream)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  int rc;
  int pfd[2];
  int flags;
  int seekable_flag;
  sigset_t chldmask;
  
  if (!fs || fs->argc == 0)
    return EINVAL;

  if (fs->pid)
    {
      _prog_close (stream);
    }

  stream_get_flags (stream, &flags);
  seekable_flag = (flags & MU_STREAM_SEEKABLE);
  
  sigemptyset (&chldmask);	/* now block SIGCHLD */
  sigaddset (&chldmask, SIGCHLD);
  
  rc = start_program_filter (&fs->pid, pfd, fs->argc, fs->argv, NULL);
  if (rc)
    return rc;

  if (flags & (MU_STREAM_READ|MU_STREAM_RDWR))
    {
      FILE *fp = fdopen (pfd[0], "r");
      rc = stdio_stream_create (&fs->in, fp, MU_STREAM_READ|seekable_flag);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
      rc = stream_open (fs->in);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
    }
  else
    close (pfd[0]);
  
  if (flags & (MU_STREAM_WRITE|MU_STREAM_RDWR))
    {
      FILE *fp = fdopen (pfd[1], "w");
      rc = stdio_stream_create (&fs->out, fp, MU_STREAM_WRITE|seekable_flag);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
      rc = stream_open (fs->out);
      if (rc)
	{
	  _prog_close (stream);
	  return rc;
	}
    }
  else
    close (pfd[1]);

  _prog_stream_register (fs);
  if (fs->input)
    return feed_input (fs);
  return 0;
}

static int
_prog_read (stream_t stream, char *optr, size_t osize,
	    off_t offset, size_t *pnbytes)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  return stream_read (fs->in, optr, osize, offset, pnbytes);
}

static int
_prog_readline (stream_t stream, char *optr, size_t osize,
		off_t offset, size_t *pnbytes)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  return stream_readline (fs->in, optr, osize, offset, pnbytes);
}

static int
_prog_write (stream_t stream, const char *iptr, size_t isize,
	     off_t offset, size_t *pnbytes)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  return stream_write (fs->out, iptr, isize, offset, pnbytes);
}

static int
_prog_flush (stream_t stream)
{
  struct _prog_stream *fs = stream_get_owner (stream);
  stream_flush (fs->in);
  stream_flush (fs->out);
  return 0;
}

static int
_prog_get_fd (stream_t stream, int *pfd, int *pfd2)
{
  int rc;
  struct _prog_stream *fs = stream_get_owner (stream);
  
  if ((rc = stream_get_fd (fs->in, pfd)) != 0)
    return rc;
  return stream_get_fd (fs->out, pfd2);
}

int
_prog_stream_create (struct _prog_stream **pfs,
		     stream_t *stream, char *progname, int flags)
{
  struct _prog_stream *fs;
  int ret;

  if (stream == NULL)
    return EINVAL;

  if (progname == NULL || (flags & MU_STREAM_NO_CLOSE))
    return EINVAL;

  if ((flags & (MU_STREAM_READ|MU_STREAM_WRITE)) ==
      (MU_STREAM_READ|MU_STREAM_WRITE))
    {
      flags &= ~(MU_STREAM_READ|MU_STREAM_WRITE);
      flags |= MU_STREAM_RDWR;
    }
  
  fs = calloc (1, sizeof (*fs));
  if (fs == NULL)
    return ENOMEM;
  if (argcv_get (progname, "", "#", &fs->argc, &fs->argv))
    {
      argcv_free (fs->argc, fs->argv);
      free (fs);
      return ENOMEM;
    }

  ret = stream_create (stream, flags|MU_STREAM_NO_CHECK, fs);
  if (ret != 0)
    {
      argcv_free (fs->argc, fs->argv);
      free (fs);
      return ret;
    }

  stream_set_read (*stream, _prog_read, fs);
  stream_set_readline (*stream, _prog_readline, fs);
  stream_set_write (*stream, _prog_write, fs);
  
  /* We don't need to open the FILE, just return success. */

  stream_set_open (*stream, _prog_open, fs);
  stream_set_close (*stream, _prog_close, fs);
  stream_set_fd (*stream, _prog_get_fd, fs);
  stream_set_flush (*stream, _prog_flush, fs);
  stream_set_destroy (*stream, _prog_destroy, fs);

  if (pfs)
    *pfs = fs;
  return 0;
}

int
prog_stream_create (stream_t *stream, char *progname, int flags)
{
  return _prog_stream_create (NULL, stream, progname, flags);
}

int
filter_prog_stream_create (stream_t *stream, char *progname, stream_t input)
{
  struct _prog_stream *fs;
  int rc = _prog_stream_create (&fs, stream, progname, MU_STREAM_RDWR);
  if (rc)
    return rc;
  fs->input = input;

  return 0;
}

