/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_STREAM_H
#define _MAILUTILS_STREAM_H

#include <mailutils/types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

enum mu_buffer_type
  {
    mu_buffer_none,
    mu_buffer_line,
    mu_buffer_full
  };

#define MU_SEEK_SET      0
#define MU_SEEK_CUR      1
#define MU_SEEK_END      2

#define MU_STREAM_READ	      0x00000001
#define MU_STREAM_WRITE	      0x00000002
#define MU_STREAM_RDWR        (MU_STREAM_READ|MU_STREAM_WRITE)
#define MU_STREAM_SEEK        0x00000004
#define MU_STREAM_APPEND      0x00000008
#define MU_STREAM_CREAT	      0x00000010
/* So far used only by TCP streams. */
#define MU_STREAM_NONBLOCK    0x00000020
#define MU_STREAM_AUTOCLOSE   0x00000040
/* Not used. Intended for mailboxes only. */
#define MU_STREAM_NONLOCK     0x00000080
#define MU_STREAM_ALLOW_LINKS 0x00000100
/* FIXME: This one affects only mailboxes */  
#define MU_STREAM_QACCESS     0x00000200

#define MU_STREAM_RDTHRU      0x00000400
#define MU_STREAM_WRTHRU      0x00000800

#define MU_STREAM_IRGRP       0x00001000
#define MU_STREAM_IWGRP       0x00002000
#define MU_STREAM_IROTH       0x00004000
#define MU_STREAM_IWOTH       0x00008000
#define MU_STREAM_IMASK       0x0000F000

#define MU_IOCTL_GET_TRANSPORT   1
#define MU_IOCTL_GET_STATUS      2
#define MU_IOCTL_GET_PID         3
#define MU_IOCTL_SET_SEEK_LIMITS 4
#define MU_IOCTL_ABRIDGE_SEEK MU_IOCTL_SET_SEEK_LIMITS
#define MU_IOCTL_GET_SEEK_LIMITS 5
#define MU_IOCTL_SET_TRANSPORT   6
#define MU_IOCTL_GET_STREAM      7
#define MU_IOCTL_SET_STREAM      8  

#define MU_IOCTL_LEVEL           9

#define MU_IOCTL_GET_TRANSPORT_BUFFER 10
#define MU_IOCTL_SET_TRANSPORT_BUFFER 11

#define MU_TRANSPORT_INPUT  0
#define MU_TRANSPORT_OUTPUT 1
#define MU_TRANSPORT_VALID_TYPE(n) \
  ((n) == MU_TRANSPORT_INPUT || (n) == MU_TRANSPORT_OUTPUT)

struct mu_buffer_query
{
  int type;                     /* One of MU_TRANSPORT_ defines */
  enum mu_buffer_type buftype;  /* Buffer type */
  size_t bufsize;               /* Buffer size */
};

#define MU_STREAM_DEFBUFSIZ 8192
extern size_t mu_stream_default_buffer_size;

void mu_stream_ref (mu_stream_t stream);
void mu_stream_unref (mu_stream_t stream);
void mu_stream_destroy (mu_stream_t *pstream);
int mu_stream_open (mu_stream_t stream);
const char *mu_stream_strerror (mu_stream_t stream, int rc);
int mu_stream_err (mu_stream_t stream);
int mu_stream_last_error (mu_stream_t stream);
void mu_stream_clearerr (mu_stream_t stream);
int mu_stream_seterr (struct _mu_stream *stream, int code, int perm);

int mu_stream_eof (mu_stream_t stream);
int mu_stream_seek (mu_stream_t stream, mu_off_t offset, int whence,
		    mu_off_t *pres);
int mu_stream_skip_input_bytes (mu_stream_t stream, mu_off_t count,
				mu_off_t *pres);

int mu_stream_set_buffer (mu_stream_t stream, enum mu_buffer_type type,
			  size_t size);
int mu_stream_get_buffer (mu_stream_t stream,
			  struct mu_buffer_query *qry);
int mu_stream_read (mu_stream_t stream, void *buf, size_t size, size_t *pread);
int mu_stream_readdelim (mu_stream_t stream, char *buf, size_t size,
			 int delim, size_t *pread);
int mu_stream_readline (mu_stream_t stream, char *buf, size_t size, size_t *pread);
int mu_stream_getdelim (mu_stream_t stream, char **pbuf, size_t *psize,
			int delim, size_t *pread);
int mu_stream_getline (mu_stream_t stream, char **pbuf, size_t *psize,
		       size_t *pread);
int mu_stream_write (mu_stream_t stream, const void *buf, size_t size,
		     size_t *pwrite);
int mu_stream_writeline (mu_stream_t stream, const char *buf, size_t size);
int mu_stream_flush (mu_stream_t stream);
int mu_stream_close (mu_stream_t stream);
int mu_stream_size (mu_stream_t stream, mu_off_t *psize);
mu_off_t mu_stream_bytes_in (mu_stream_t stream);
mu_off_t mu_stream_bytes_out (mu_stream_t stream);
int mu_stream_ioctl (mu_stream_t stream, int code, void *ptr);
int mu_stream_truncate (mu_stream_t stream, mu_off_t);
int mu_stream_shutdown (mu_stream_t stream, int how);

#define MU_STREAM_READY_RD 0x1
#define MU_STREAM_READY_WR 0x2
#define MU_STREAM_READY_EX 0x4  
struct timeval;  /* Needed for the following declaration */ 

int mu_stream_wait   (mu_stream_t stream, int *pflags, struct timeval *);

void mu_stream_get_flags (mu_stream_t stream, int *pflags);
int mu_stream_set_flags (mu_stream_t stream, int fl);
int mu_stream_clr_flags (mu_stream_t stream, int fl);

int mu_stream_vprintf (mu_stream_t str, const char *fmt, va_list ap);
int mu_stream_printf (mu_stream_t stream, const char *fmt, ...)
                      MU_PRINTFLIKE(2,3);

int mu_stream_copy (mu_stream_t dst, mu_stream_t src, mu_off_t size,
		    mu_off_t *pcsz);


int mu_file_stream_create (mu_stream_t *pstream, const char *filename, int flags);
int mu_temp_file_stream_create (mu_stream_t *pstream, const char *dir);
int mu_fd_stream_create (mu_stream_t *pstream, char *filename, int fd,
			 int flags);

#define MU_STDIN_FD  0
#define MU_STDOUT_FD 1
#define MU_STDERR_FD 2
int mu_stdio_stream_create (mu_stream_t *pstream, int fd, int flags);

int mu_prog_stream_create (mu_stream_t *pstream, const char *progname, int flags);
int mu_filter_prog_stream_create (mu_stream_t *pstream, const char *progname,
				  mu_stream_t input);
int mu_memory_stream_create (mu_stream_t *pstream, int flags);
int mu_mapfile_stream_create (mu_stream_t *pstream, const char *filename,
			      int flags);
int mu_socket_stream_create (mu_stream_t *pstream, const char *filename,
			     int flags);

int mu_streamref_create_abridged (mu_stream_t *pref, mu_stream_t str,
				  mu_off_t start, mu_off_t end);
int mu_streamref_create (mu_stream_t *pref, mu_stream_t str);

int mu_tcp_stream_create_with_source_ip (mu_stream_t *stream,
					 const char *host, int port,
					 unsigned long source_ip,
					 int flags);
int mu_tcp_stream_create_with_source_host (mu_stream_t *stream,
					   const char *host, int port,
					   const char *source_host,
					   int flags);
int mu_tcp_stream_create (mu_stream_t *stream, const char *host, int port,
			  int flags);

/* Transcript output levels */
#define MU_XSCRIPT_NORMAL  0  /* Normal transcript */
#define MU_XSCRIPT_SECURE  1  /* Security-related data are being transmitted */
#define MU_XSCRIPT_PAYLOAD 2  /* Actual payload (may be copious) */

int mu_xscript_stream_create(mu_stream_t *pref, mu_stream_t transport,
			     mu_stream_t logstr,
			     const char *prefix[]);

int mu_iostream_create (mu_stream_t *pref, mu_stream_t in, mu_stream_t out);
int mu_dbgstream_create(mu_stream_t *pref, mu_debug_t debug,
			mu_log_level_t level, int flags);

int mu_rdcache_stream_create (mu_stream_t *pstream, mu_stream_t transport,
			      int flags);

#ifdef __cplusplus
}
#endif
  
#endif
