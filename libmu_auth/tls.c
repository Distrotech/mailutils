/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2004, 2007-2012 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <mailutils/error.h>
#include <mailutils/mu_auth.h>
#include <mailutils/tls.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>

struct mu_tls_module_config mu_tls_module_config = {
#ifdef WITH_TLS
  1 /* enable by default */
#else
  0
#endif
};
  
int
mu_tls_module_init (enum mu_gocs_op op, void *data)
{
  switch (op)
    {
    case mu_gocs_op_set:
      if (data)
	memcpy (&mu_tls_module_config, data, sizeof mu_tls_module_config);
      break;
      
    case mu_gocs_op_flush:
#ifdef WITH_TLS
      mu_init_tls_libs (0);
#endif    
      break;
    }
  return 0;
}

#ifdef WITH_TLS

#include <gnutls/gnutls.h>
#include <mailutils/sys/tls-stream.h>

static gnutls_certificate_server_credentials x509_cred;

/* Return: zero means NOT READY, one means READY */
int
mu_check_tls_environment (void)
{
  if (!mu_tls_module_config.enable)
    return 0;
  if (mu_tls_module_config.ssl_cert && mu_tls_module_config.ssl_key)
    {
      int rc = mu_file_safety_check (mu_tls_module_config.ssl_cert,
				     mu_tls_module_config.ssl_cert_safety_checks,
				     -1, NULL);
      if (rc)
	{
	  mu_error ("%s: %s", mu_tls_module_config.ssl_cert, 
		    mu_strerror (rc));
	  return 0;
	}
      rc = mu_file_safety_check (mu_tls_module_config.ssl_key,
				 mu_tls_module_config.ssl_key_safety_checks,
				 -1, NULL);
      if (rc)
	{
	  mu_error ("%s: %s", mu_tls_module_config.ssl_key, 
		    mu_strerror (rc));
	  return 0;
	}

      if (mu_tls_module_config.ssl_cafile)
	{
	  rc = mu_file_safety_check (mu_tls_module_config.ssl_cafile,
			       mu_tls_module_config.ssl_cafile_safety_checks,
				     -1, NULL);
	  if (rc)
	    {
	      mu_error ("%s: %s", mu_tls_module_config.ssl_cafile, 
			mu_strerror (rc));
	      return 0;
	    }
	}
    }
  else
    return 0;

  return 1;
}

int mu_tls_enable = 0;

#ifdef DEBUG_TLS
void
_mu_gtls_logger(int level, const char *text)
{
  mu_diag_output (MU_DIAG_DEBUG, "GnuTLS(%d): %s", level, text);
}
#endif

int
mu_init_tls_libs (int x509_setup)
{
  if (!mu_tls_enable)
    {
      int rc;
      if ((rc = gnutls_global_init ()) == GNUTLS_E_SUCCESS)
	mu_tls_enable = 1;
      else
	{
	  mu_error ("gnutls_global_init: %s", gnutls_strerror (rc));
	  return 0;
	}
    }

  if (x509_setup && !x509_cred)
    {
      gnutls_certificate_allocate_credentials (&x509_cred);
      if (mu_tls_module_config.ssl_cafile)
	gnutls_certificate_set_x509_trust_file (x509_cred,
						mu_tls_module_config.ssl_cafile,
						GNUTLS_X509_FMT_PEM);
  
      gnutls_certificate_set_x509_key_file (x509_cred,
					    mu_tls_module_config.ssl_cert, 
					    mu_tls_module_config.ssl_key,
					    GNUTLS_X509_FMT_PEM);
    }
  
#ifdef DEBUG_TLS
  gnutls_global_set_log_function (_mu_gtls_logger);
  gnutls_global_set_log_level (110);
#endif
  return mu_tls_enable;
}

void
mu_deinit_tls_libs (void)
{
  if (mu_tls_enable)
    {
      if (x509_cred)
	gnutls_certificate_free_credentials (x509_cred);
      gnutls_global_deinit ();
    }
  mu_tls_enable = 0;
}

static gnutls_session
initialize_tls_session (void)
{
  gnutls_session session = 0;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_set_default_priority (session);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, x509_cred);
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);

  return session;
}


/* ************************* TLS Stream Support **************************** */

static int
_tls_io_close (mu_stream_t stream)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  
  return mu_stream_close (sp->transport);
}

static void
_tls_io_done (struct _mu_stream *stream)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  mu_stream_unref (sp->transport);
}

static int
_tls_io_flush (struct _mu_stream *stream)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  return mu_stream_flush (sp->transport);
}

static int
_tls_io_read (struct _mu_stream *stream, char *buf, size_t bufsize,
	      size_t *pnread)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  int rc;
  
  if (sp->up->state != state_open)
    return EINVAL;
  rc = gnutls_record_recv (sp->up->session, buf, bufsize);
  if (rc >= 0)
    {
      *pnread = rc;
      return 0;
    }
  sp->up->tls_err = rc;
  return EIO;
}

static int
_tls_io_write (struct _mu_stream *stream, const char *buf, size_t bufsize,
	    size_t *pnwrite)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  int rc;
  
  if (sp->up->state != state_open)
    return EINVAL;

  /* gnutls_record_send() docs say:
       If the EINTR is returned by the internal push function (write())
       then GNUTLS_E_INTERRUPTED, will be returned. If GNUTLS_E_INTERRUPTED or
       GNUTLS_E_AGAIN is returned you must call this function again, with the
       same parameters. Otherwise the write operation will be
       corrupted and the connection will be terminated. */
    
  do
    rc = gnutls_record_send (sp->up->session, buf, bufsize);
  while (rc == GNUTLS_E_INTERRUPTED || rc == GNUTLS_E_AGAIN);

  if (rc < 0)
    {
      sp->up->tls_err = rc;
      return EIO;
    }

  *pnwrite = rc;

  return 0;
}

static int
_tls_rd_wait (struct _mu_stream *stream, int *pflags, struct timeval *tvp)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  int rc = EINVAL;
  
  if (*pflags == MU_STREAM_READY_RD)
    rc = mu_stream_wait (sp->transport, pflags, tvp);
  return rc;
}

static int
_tls_wr_wait (struct _mu_stream *stream, int *pflags, struct timeval *tvp)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;
  int rc = EINVAL;
  
  if (*pflags == MU_STREAM_READY_WR)
    rc = mu_stream_wait (sp->transport, pflags, tvp);
  return rc;
}

static int
_tls_io_ioctl (struct _mu_stream *stream, int code, int opcode, void *arg)
{
  struct _mu_tls_io_stream *sp = (struct _mu_tls_io_stream *) stream;

  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) sp->transport;
	      ptrans[1] = NULL;
	      break;

	    case MU_IOCTL_OP_SET:
	      return ENOSYS;

	    default:
	      return EINVAL;
	    }
	}
      break;

    default:
      return ENOSYS;
    }
  return 0;
}

static int
_mu_tls_io_stream_create (mu_stream_t *pstream,
			  mu_stream_t transport, int flags,
			  struct _mu_tls_stream *master)
{
  struct _mu_tls_io_stream *sp;

  sp = (struct _mu_tls_io_stream *)
    _mu_stream_create (sizeof (*sp), (flags & MU_STREAM_RDWR) | _MU_STR_OPEN);
  if (!sp)
    return ENOMEM;

  if (flags & MU_STREAM_READ)
    {
      sp->stream.read = _tls_io_read; 
      sp->stream.wait = _tls_rd_wait;
      mu_stream_set_buffer ((mu_stream_t) sp, mu_buffer_full, 0);
    }
  else
    {
      sp->stream.write = _tls_io_write;
      sp->stream.wait = _tls_wr_wait;
      mu_stream_set_buffer ((mu_stream_t) sp, mu_buffer_line, 0);
    }
  sp->stream.flush = _tls_io_flush;
  sp->stream.close = _tls_io_close;
  sp->stream.done = _tls_io_done; 
  sp->stream.ctl = _tls_io_ioctl;

  mu_stream_ref (transport);
  sp->transport = transport;
  sp->up = master;
  *pstream = (mu_stream_t) sp;
  return 0;
}


static ssize_t
_tls_stream_pull (gnutls_transport_ptr fd, void *buf, size_t size)
{
  mu_stream_t stream = fd;
  int rc;
  size_t rdbytes;
	
  while ((rc = mu_stream_read (stream, buf, size, &rdbytes)) == EAGAIN)
    ;
  
  if (rc)
    return -1;
  return rdbytes;
}

static ssize_t
_tls_stream_push (gnutls_transport_ptr fd, const void *buf, size_t size)
{
  mu_stream_t stream = fd;
  int rc;

  rc = mu_stream_write (stream, buf, size, &size);
  if (rc)
    {
      mu_error ("_tls_stream_push: %s",
		mu_stream_strerror (stream, rc)); /* FIXME */
      return -1;
    }

  mu_stream_flush (stream);
  return size;
}


static int
_tls_server_open (mu_stream_t stream)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  int rc = 0;
  mu_transport_t transport[2];
  
  if (!mu_tls_module_config.enable)
    return MU_ERR_DISABLED;
  if (!stream || sp->state != state_init)
    return EINVAL;

  mu_init_tls_libs (1);
  
  sp->session = initialize_tls_session ();
  mu_stream_ioctl (stream, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, transport);
  gnutls_transport_set_ptr2 (sp->session,
			     (gnutls_transport_ptr) transport[0],
			     (gnutls_transport_ptr) transport[1]);
  gnutls_transport_set_pull_function (sp->session, _tls_stream_pull);
  gnutls_transport_set_push_function (sp->session, _tls_stream_push);
  
  rc = gnutls_handshake (sp->session);
  if (rc < 0)
    {
      gnutls_deinit (sp->session);
      sp->tls_err = rc;
      return EIO;
    }
  sp->state = state_open;
  return 0;
}

static int
prepare_client_session (mu_stream_t stream)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  int rc;
  mu_transport_t transport[2];
  static int protocol_priority[] = {GNUTLS_TLS1, GNUTLS_SSL3, 0};
  static int kx_priority[] = {GNUTLS_KX_RSA, 0};
  static int cipher_priority[] = {GNUTLS_CIPHER_3DES_CBC,
				  GNUTLS_CIPHER_ARCFOUR_128,
				  0};
  static int comp_priority[] = {GNUTLS_COMP_NULL, 0};
  static int mac_priority[] = {GNUTLS_MAC_SHA, GNUTLS_MAC_MD5, 0};

  gnutls_init (&sp->session, GNUTLS_CLIENT);
  gnutls_protocol_set_priority (sp->session, protocol_priority);
  gnutls_cipher_set_priority (sp->session, cipher_priority);
  gnutls_compression_set_priority (sp->session, comp_priority);
  gnutls_kx_set_priority (sp->session, kx_priority);
  gnutls_mac_set_priority (sp->session, mac_priority);

  gnutls_certificate_allocate_credentials (&x509_cred);
  if (mu_tls_module_config.ssl_cafile)
    {
      rc = gnutls_certificate_set_x509_trust_file (x509_cred,
					      mu_tls_module_config.ssl_cafile,
						   GNUTLS_X509_FMT_PEM);
      if (rc < 0)
	{
	  sp->tls_err = rc;
	  return -1;
	}
    }

  gnutls_credentials_set (sp->session, GNUTLS_CRD_CERTIFICATE, x509_cred);

  mu_stream_ioctl (stream, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET, transport);
  gnutls_transport_set_ptr2 (sp->session,
			     (gnutls_transport_ptr) transport[0],
			     (gnutls_transport_ptr) transport[1]);
  gnutls_transport_set_pull_function (sp->session, _tls_stream_pull);
  gnutls_transport_set_push_function (sp->session, _tls_stream_push);
      
  return 0;
}

static int
_tls_client_open (mu_stream_t stream)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  int rc = 0;
  
  switch (sp->state)
    {
    case state_closed:
      if (sp->session)
	gnutls_deinit (sp->session);
      /* FALLTHROUGH */
      
    case state_init:
      mu_init_tls_libs (0);
      prepare_client_session (stream);
      rc = gnutls_handshake (sp->session);
      if (rc < 0)
	{
	  sp->tls_err = rc;
	  gnutls_deinit (sp->session);
	  sp->state = state_init;
	  return MU_ERR_FAILURE;
	}
      break;

    default:
      return MU_ERR_FAILURE;
    }

  /* FIXME: if (ssl_cafile) verify_certificate (s->session); */
  sp->state = state_open;
  return 0;
}

static int
_tls_read (struct _mu_stream *str, char *buf, size_t bufsize,
	   size_t *pnread)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *)str;
  return mu_stream_read (sp->transport[0], buf, bufsize, pnread);
}

static int
_tls_write (struct _mu_stream *str, const char *buf, size_t bufsize,
	    size_t *pnwrite)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *)str;
  return mu_stream_write (sp->transport[1], buf, bufsize, pnwrite);
}

static int
_tls_ioctl (struct _mu_stream *stream, int code, int opcode, void *arg)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;

  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      switch (opcode)
	{
	case MU_IOCTL_OP_GET:
	  if (!arg)
	    return EINVAL;
	  else
	    {
	      mu_transport_t *ptrans, trans[2];

	      ptrans = arg;
	      mu_stream_ioctl (sp->transport[0], MU_IOCTL_TRANSPORT,
			       MU_IOCTL_OP_GET, trans);
	      ptrans[0] = trans[0];
	      mu_stream_ioctl (sp->transport[1], MU_IOCTL_TRANSPORT,
			       MU_IOCTL_OP_GET, trans);
	      ptrans[1] = trans[0];
	    }
	  break;

	case MU_IOCTL_OP_SET:
	  return ENOSYS;

	default:
	  return EINVAL;
	}
      break;

    case MU_IOCTL_TRANSPORT_BUFFER:
      if (!arg)
	return EINVAL;
      else
	{
	  struct mu_buffer_query *qp = arg;
	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      if (!MU_TRANSPORT_VALID_TYPE (qp->type) ||
		  !sp->transport[qp->type])
		return EINVAL;
	      return mu_stream_get_buffer (sp->transport[qp->type], qp);

	    case MU_IOCTL_OP_SET:
	      if (!MU_TRANSPORT_VALID_TYPE (qp->type) ||
		  !sp->transport[qp->type])
		return EINVAL;
	      return mu_stream_set_buffer (sp->transport[qp->type],
					   qp->buftype, qp->bufsize);

	    default:
	      return EINVAL;
	    }
	}
      break;
      
    default:
      return ENOSYS;
    }
  return 0;
}

static int
_tls_wait (struct _mu_stream *stream, int *pflags, struct timeval *tvp)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  int rc = EINVAL;
  
  if (*pflags == MU_STREAM_READY_RD)
    rc = mu_stream_wait (sp->transport[0], pflags, tvp);
  else if (*pflags == MU_STREAM_READY_WR)
    rc = mu_stream_wait (sp->transport[1], pflags, tvp);
  return rc;
}

static int
_tls_flush (struct _mu_stream *stream)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  return mu_stream_flush (sp->transport[1]);
}

static int
_tls_close (mu_stream_t stream)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  
  if (sp->session && sp->state == state_open)
    {
      gnutls_bye (sp->session, GNUTLS_SHUT_RDWR);
      sp->state = state_closed;
    }
  
  mu_stream_close (sp->transport[0]);
  mu_stream_close (sp->transport[1]);
  return 0;
}

static void
_tls_done (struct _mu_stream *stream)
{
  struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
  
  if (sp->session && sp->state == state_closed)
    {
      gnutls_deinit (sp->session);
      sp->state = state_destroyed;
    }

  mu_stream_destroy (&sp->transport[0]);
  mu_stream_destroy (&sp->transport[1]);
}

static const char *
_tls_error_string (struct _mu_stream *stream, int rc)
{
  if (rc == EIO)
    {
      struct _mu_tls_stream *sp = (struct _mu_tls_stream *) stream;
      return gnutls_strerror (sp->tls_err);
    }
  return mu_strerror (rc);
}

static int
_mu_tls_stream_create (mu_stream_t *pstream,
		       int (*openfn) (mu_stream_t stream),
		       mu_stream_t strin, mu_stream_t strout, int flags)
{
  struct _mu_tls_stream *sp;
  int rc;
  mu_stream_t stream;
  
  sp = (struct _mu_tls_stream *)
    _mu_stream_create (sizeof (*sp), MU_STREAM_RDWR);
  if (!sp)
    return ENOMEM;

  sp->stream.read = _tls_read; 
  sp->stream.write = _tls_write;
  sp->stream.flush = _tls_flush;
  sp->stream.open = openfn; 
  sp->stream.close = _tls_close;
  sp->stream.done = _tls_done; 
  sp->stream.ctl = _tls_ioctl;
  sp->stream.wait = _tls_wait;
  sp->stream.error_string = _tls_error_string;

  mu_stream_set_buffer (strin, mu_buffer_none, 0);
  mu_stream_set_buffer (strout, mu_buffer_none, 0);
  rc = _mu_tls_io_stream_create (&sp->transport[0], strin, MU_STREAM_READ, sp);
  if (rc)
    {
      free (sp);
      return rc;
    }
     
  rc = _mu_tls_io_stream_create (&sp->transport[1], strout, MU_STREAM_WRITE,
                                 sp);
  if (rc)
    {
      free (sp);
      free (sp->transport[0]);
      return rc;
    }

  stream = (mu_stream_t) sp;
  mu_stream_set_buffer (stream, mu_buffer_line, 0);
  rc = mu_stream_open (stream);
  if (rc)
    mu_stream_destroy (&stream);
  else
    *pstream = stream;
  return rc;
}

int
mu_tls_server_stream_create (mu_stream_t *pstream,
			     mu_stream_t strin, mu_stream_t strout, int flags)
{
  return _mu_tls_stream_create (pstream,
				_tls_server_open,
				strin, strout, flags);
}

int
mu_tls_client_stream_create (mu_stream_t *pstream,
			     mu_stream_t strin, mu_stream_t strout, int flags)
{
  return _mu_tls_stream_create (pstream,
				_tls_client_open,
				strin, strout, flags);
}


#endif /* WITH_TLS */

/* EOF */

