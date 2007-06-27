/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef WITH_GSASL

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <mailutils/argp.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/mu_auth.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>

#include <gsasl.h>
#include <lbuf.h>

char *mu_gsasl_cram_md5_pwd = SITE_CRAM_MD5_PWD;

#define ARG_CRAM_PASSWD 1

static struct argp_option _gsasl_argp_options[] = {
  {NULL, 0, NULL, 0, N_("GSASL options"), 0},
  {"cram-passwd", ARG_CRAM_PASSWD, N_("FILE"), 0,
   N_("Specify password file for CRAM-MD5 authentication"), 0},
  { NULL,      0, NULL, 0, NULL, 0 }
};

static error_t
_gsasl_argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARG_CRAM_PASSWD:
      mu_gsasl_cram_md5_pwd = arg;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp _gsasl_argp = {
  _gsasl_argp_options,
  _gsasl_argp_parser
};

static struct argp_child _gsasl_argp_child = {
  &_gsasl_argp,
  0,
  NULL,
  0
};

void
mu_gsasl_init_argp ()
{
  if (mu_register_capa ("gsasl", &_gsasl_argp_child))
    {
      mu_error (_("INTERNAL ERROR: cannot register argp capability gsasl"));
      abort ();
    }
}


struct _gsasl_stream {
  Gsasl_session_ctx *sess_ctx; /* Context */
  int last_err;        /* Last Gsasl error code */
  
  mu_stream_t stream;     /* I/O stream */
  struct _line_buffer *lb; 
};

static void
_gsasl_destroy (mu_stream_t stream)
{
  int flags;
  struct _gsasl_stream *s = mu_stream_get_owner (stream);
  mu_stream_get_flags (stream, &flags);
  if (!(flags & MU_STREAM_NO_CLOSE))
    mu_stream_destroy (&s->stream, mu_stream_get_owner (s->stream));
  _auth_lb_destroy (&s->lb);
}

static int
_gsasl_readline (mu_stream_t stream, char *optr, size_t osize,
		 off_t offset, size_t *nbytes)
{
  struct _gsasl_stream *s = mu_stream_get_owner (stream);
  int rc;
  size_t len, sz;
  char *bufp = NULL;
  
  if (_auth_lb_level (s->lb))
    {
      len = _auth_lb_readline (s->lb, optr, osize-1);
      optr[len] = 0;
      if (nbytes)
	*nbytes = len;
      return 0;
    }

  do
    {
      char buf[80];
      size_t sz;
      int status;
      
      status = mu_stream_sequential_read (s->stream, buf, sizeof (buf), &sz);
      if (status == EINTR)
	continue;
      else if (status)
	{
	  free (bufp);
	  return status;
	}
      rc = _auth_lb_grow (s->lb, buf, sz);
      if (rc)
	return rc;

      rc = gsasl_decode (s->sess_ctx,
			 _auth_lb_data (s->lb),
			 _auth_lb_level (s->lb),
			 &bufp, &len);
    }
  while (rc == GSASL_NEEDS_MORE);

  if (rc != GSASL_OK)
    {
      s->last_err = rc;
      free (bufp);
      return EIO;
    }
      
  sz = len > osize ? osize : len;
  
  if (len > osize)
    {
      memcpy (optr, bufp, osize);
      _auth_lb_drop (s->lb);
      _auth_lb_grow (s->lb, bufp + osize, len - osize);
      len = osize;
    }
  else
    {
      _auth_lb_drop (s->lb);
      memcpy (optr, bufp, len);
    }

  if (len < osize)
    optr[len] = 0;
  
  if (nbytes)
    *nbytes = len;
  
  free (bufp);

  return 0;
}

int
write_chunk (void *data, char *start, char *end)
{
  struct _gsasl_stream *s = data;
  size_t chunk_size = end - start + 1;
  size_t len = 0;
  char *buf = NULL;
  int status;
    
  gsasl_encode (s->sess_ctx, start, chunk_size, &buf, &len);

  status = mu_stream_sequential_write (s->stream, buf, len);
  
  free (buf);

  return status;
}


static int
_gsasl_write (mu_stream_t stream, const char *iptr, size_t isize,
	      off_t offset, size_t *nbytes)
{
  int rc;
  struct _gsasl_stream *s = mu_stream_get_owner (stream);
  
  rc = _auth_lb_grow (s->lb, iptr, isize);
  if (rc)
    return rc;

  return _auth_lb_writelines (s->lb, iptr, isize, offset,
			      write_chunk, s, nbytes);      
}

static int
_gsasl_flush (mu_stream_t stream)
{
  struct _gsasl_stream *s = mu_stream_get_owner (stream);
  return mu_stream_flush (s->stream);
}

static int
_gsasl_close (mu_stream_t stream)
{
  int flags;
  struct _gsasl_stream *s = mu_stream_get_owner (stream);

  mu_stream_get_flags (stream, &flags);
  if (!(flags & MU_STREAM_NO_CLOSE))
    mu_stream_close (s->stream);
  return 0;
}

static int
_gsasl_open (mu_stream_t stream)
{
  /* Nothing to do */
  return 0;
}

int
_gsasl_strerror (mu_stream_t stream, const char **pstr)
{
  struct _gsasl_stream *s = mu_stream_get_owner (stream);
  *pstr = gsasl_strerror (s->last_err);
  return 0;
}

int
_gsasl_get_transport2 (mu_stream_t stream, mu_transport_t *pt, mu_transport_t *pt2)
{
  struct _gsasl_stream *s = mu_stream_get_owner (stream);
  *pt2 = NULL; /* FIXME 1 */
  *pt = (mu_transport_t) s->stream;
  return 0;
}

int
_gsasl_wait (mu_stream_t stream, int *pflags, struct timeval *tvp)
{
  int flags;
  struct _gsasl_stream *s = mu_stream_get_owner (stream);

  mu_stream_get_flags (stream, &flags);
  if (((*pflags & MU_STREAM_READY_RD) && !(flags & MU_STREAM_READ))
      || ((*pflags & MU_STREAM_READY_WR) && !(flags & MU_STREAM_WRITE)))
    return EINVAL; 
  return mu_stream_wait (s->stream, pflags, tvp);
}

int
mu_gsasl_stream_create (mu_stream_t *stream, mu_stream_t transport,
		     Gsasl_session_ctx *ctx, int flags)
{
  struct _gsasl_stream *s;
  int rc;
    
  if (stream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if ((flags & ~(MU_STREAM_READ|MU_STREAM_WRITE))
      || (flags & (MU_STREAM_READ|MU_STREAM_WRITE)) ==
          (MU_STREAM_READ|MU_STREAM_WRITE))
    return EINVAL;
  
  s = calloc (1, sizeof (*s));
  if (s == NULL)
    return ENOMEM;

  s->stream = transport;
  s->sess_ctx = ctx;
  
  rc = mu_stream_create (stream, flags|MU_STREAM_NO_CHECK, s);
  if (rc)
    {
      free (s);
      return rc;
    }

  mu_stream_set_open (*stream, _gsasl_open, s);
  mu_stream_set_close (*stream, _gsasl_close, s);
  mu_stream_set_flush (*stream, _gsasl_flush, s);
  mu_stream_set_destroy (*stream, _gsasl_destroy, s);
  mu_stream_set_strerror (*stream, _gsasl_strerror, s);
  mu_stream_set_wait (*stream, _gsasl_wait, s);
  mu_stream_set_get_transport2 (*stream, _gsasl_get_transport2, s);
  if (flags & MU_STREAM_READ)
    mu_stream_set_readline (*stream, _gsasl_readline, s);
  else
    mu_stream_set_write (*stream, _gsasl_write, s);

  _auth_lb_create (&s->lb);
  
  return 0;
}
  
#endif
