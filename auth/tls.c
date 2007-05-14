/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <mailutils/argp.h>
#include <mailutils/error.h>
#include <mailutils/mu_auth.h>
#include <mailutils/tls.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>

#include <lbuf.h>

#ifdef WITH_TLS

#include <gnutls/gnutls.h>

#define DH_BITS 768

static gnutls_dh_params dh_params;
static gnutls_certificate_server_credentials x509_cred;
static char *ssl_cert = NULL;
static char *ssl_key = NULL;
static char *ssl_cafile = NULL;

#define ARG_TLS        1 
#define ARG_SSL_CERT   2
#define ARG_SSL_KEY    3
#define ARG_SSL_CAFILE 4

static struct argp_option _tls_argp_options[] = {
  {NULL, 0, NULL, 0, N_("Encryption options"), 0},
  {"ssl-cert", ARG_SSL_CERT, N_("FILE"), 0,
   N_("Specify SSL certificate file"), 0},
  {"ssl-key", ARG_SSL_KEY, N_("FILE"), 0,
   N_("Specify SSL certificate key"), 0},
  {"ssl-cafile", ARG_SSL_CAFILE, N_("FILE"), 0,
   N_("Specify trusted CAs file"), 0},
  {NULL, 0, NULL, 0, NULL, 0}
};

static error_t
_tls_argp_parser (int key, char *arg, struct argp_state *state)
{
  static int tls_enable = 1;
  
  switch (key)
    {
    case ARG_TLS:
      if (!arg || strcasecmp (arg, "yes") == 0)
	tls_enable = 1;
      else if (strcasecmp (arg, "no") == 0)
	tls_enable = 0;
      break;
      
    case ARG_SSL_CERT:
      ssl_cert = arg;
      break;

    case ARG_SSL_KEY:
      ssl_key = arg;
      break;

    case ARG_SSL_CAFILE:
      ssl_cafile = arg;
      break;

    case ARGP_KEY_FINI:
      if (tls_enable)
	mu_init_tls_libs ();
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp _tls_argp = {
  _tls_argp_options,
  _tls_argp_parser
};

static struct argp_child _tls_argp_child = {
  &_tls_argp,
  0,
  NULL,
  0
};

void
mu_tls_init_argp ()
{
  if (mu_register_capa ("tls", &_tls_argp_child))
    {
      mu_error (_("INTERNAL ERROR: cannot register argp capability tls"));
      abort ();
    }
}

static struct argp_option _tls_argp_client_options[] = {
  {"tls", ARG_TLS, N_("BOOL"), OPTION_ARG_OPTIONAL,
   N_("Enable TLS support") },
  {NULL, 0, NULL, 0, NULL, 0}
};
  
static struct argp _tls_client_argp = {
  _tls_argp_client_options,
  _tls_argp_parser
};

static struct argp_child _tls_argp_client_child = {
  &_tls_client_argp,
  0,
  NULL,
  0
};

void
mu_tls_init_client_argp ()
{
  if (mu_register_capa ("tls", &_tls_argp_client_child))
    {
      mu_error (_("INTERNAL ERROR: cannot register argp capability tls"));
      abort ();
    }
}

int
mu_check_tls_environment (void)
{
  /* Return: zero means NOT READY, one means READY */

  if (ssl_cert && ssl_key)
    {
      struct stat st;

      if (stat (ssl_cert, &st) == -1)
	{
	  mu_error ("%s: %s.", ssl_cert, mu_strerror (errno));
	  return 0;
	}
      if (!(st.st_mode & S_IFREG) || !(st.st_mode & S_IFLNK))
	{
	  mu_error (_("%s is not a regular file or a symbolic link."),
		    ssl_cert);
	  return 0;
	}

      if (stat (ssl_key, &st) == -1)
	{
	  mu_error ("%s: %s.", ssl_key, mu_strerror(errno));
	  return 0;
	}
      if (!(st.st_mode & S_IFREG) || !(st.st_mode & S_IFLNK))
	{
	  mu_error (_("%s is not a regular file or a symbolic link."),
		    ssl_key);
	  return 0;
	}
      if ((st.st_mode & S_IRWXG) || (st.st_mode & S_IRWXO))
	{
	  mu_error (_("Wrong permissions on %s. Set 0600"), ssl_key);
	  return 0;
	}
    }
  else
    return 0;

  return 1;
}

int mu_tls_enable = 0;

int
mu_init_tls_libs (void)
{
  if (!mu_tls_enable)
    mu_tls_enable = !gnutls_global_init (); /* Returns 1 on success */
  return mu_tls_enable;
}

void
mu_deinit_tls_libs (void)
{
  if (mu_tls_enable)
    gnutls_global_deinit ();
  mu_tls_enable = 0;
}

static void
generate_dh_params (void)
{
  gnutls_dh_params_init (&dh_params);
  gnutls_dh_params_generate2 (dh_params, DH_BITS);
}

static gnutls_session
initialize_tls_session (void)
{
  gnutls_session session = 0;

  gnutls_init (&session, GNUTLS_SERVER);
  gnutls_set_default_priority (session);
  gnutls_credentials_set (session, GNUTLS_CRD_CERTIFICATE, x509_cred);
  gnutls_certificate_server_set_request (session, GNUTLS_CERT_REQUEST);
  gnutls_dh_set_prime_bits (session, DH_BITS);

  return session;
}

/* ************************* TLS Stream Support **************************** */

enum tls_stream_state {
  state_init,
  state_open,
  state_closed,
  state_destroyed
};

struct _tls_stream {
  mu_stream_t strin;  /* Input stream */
  mu_stream_t strout; /* Output stream */ 
  int last_err;
  struct _line_buffer *lb;
  enum tls_stream_state state;
  gnutls_session session;
};


static void
_tls_destroy (mu_stream_t stream)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  if (x509_cred)
    gnutls_certificate_free_credentials (x509_cred);
  if (s->session && s->state == state_closed)
    {
      gnutls_deinit (s->session);
      s->state = state_destroyed;
    }
  _auth_lb_destroy (&s->lb);
  free (s);
}
    
static int
_tls_read (mu_stream_t stream, char *optr, size_t osize,
	   mu_off_t offset, size_t *nbytes)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  int rc;
  
  if (!stream || s->state != state_open)
    return EINVAL;
  rc = gnutls_record_recv (s->session, optr, osize);
  if (rc >= 0)
    {
      *nbytes = rc;
      return 0;
    }
  s->last_err = rc;
  return EIO;
}

static int
_tls_readline (mu_stream_t stream, char *optr, size_t osize,
		mu_off_t offset, size_t *nbytes)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  int rc;
  char *ptr;
  size_t rdsize;
  
  if (!stream || s->state != state_open || osize < 2)
    return EINVAL;

  if (_auth_lb_level (s->lb) == 0)
    {
      ptr = optr;
      rdsize = 0;
      do
	{
	  rc = gnutls_record_recv (s->session, ptr + rdsize, osize - rdsize);
	  if (rc < 0)
	    {
	      s->last_err = rc;
	      return EIO;
	    }
	  rdsize += rc;
	}
      while (osize > rdsize && rc > 0 && ptr[rdsize-1] != '\n');
      
      _auth_lb_grow (s->lb, ptr, rdsize);
    }
  
  osize--; /* Allow for terminating zero */
  rdsize = _auth_lb_readline (s->lb, optr, osize);
  optr[rdsize] = 0;
  if (nbytes)
    *nbytes = rdsize;
  return 0;
}

static int
_tls_write (mu_stream_t stream, const char *iptr, size_t isize,
	    mu_off_t offset, size_t *nbytes)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  int rc;
  
  if (!stream || s->state != state_open)
    return EINVAL;

  /* gnutls_record_send() docs say:
       If the EINTR is returned by the internal push function (write())
       then GNUTLS_E_INTERRUPTED, will be returned. If GNUTLS_E_INTERRUPTED or
       GNUTLS_E_AGAIN is returned you must call this function again, with the
       same parameters. Otherwise the write operation will be
       corrupted and the connection will be terminated. */
    
  do
    rc = gnutls_record_send (s->session, iptr, isize);
  while (rc == GNUTLS_E_INTERRUPTED || rc == GNUTLS_E_AGAIN);

  if (rc < 0)
    {
      s->last_err = rc;
      return EIO;
    }

  if (nbytes)
    *nbytes = rc;

  return 0;
}

static int
_tls_flush (mu_stream_t stream)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  return mu_stream_flush (s->strout);
}

static int
_tls_close (mu_stream_t stream)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  if (s->session && s->state == state_open)
    {
      gnutls_bye (s->session, GNUTLS_SHUT_RDWR);
      s->state = state_closed;
    }
  return 0;
}


/* Push & pull functions */

static ssize_t
_tls_stream_pull (gnutls_transport_ptr fd, void *buf, size_t size)
{
  mu_stream_t stream = fd;
  int rc;
  size_t rdbytes;
	
  while ((rc = mu_stream_sequential_read (stream, buf, size, &rdbytes)) == EAGAIN)
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

  rc = mu_stream_sequential_write (stream, buf, size);
  if (rc)
    return -1;
  mu_stream_flush (stream);
  return size;
}



static int
_tls_open (mu_stream_t stream)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  int rc = 0;
  
  if (!stream || s->state != state_init)
    return EINVAL;

  gnutls_certificate_allocate_credentials (&x509_cred);

  if (ssl_cafile)
    gnutls_certificate_set_x509_trust_file (x509_cred, ssl_cafile,
					    GNUTLS_X509_FMT_PEM);

  rc = gnutls_certificate_set_x509_key_file (x509_cred,
					     ssl_cert, ssl_key,
					     GNUTLS_X509_FMT_PEM);
  if (rc < 0)
    {
      s->last_err = rc;
      return EIO;
    }
  
  generate_dh_params ();
  gnutls_certificate_set_dh_params (x509_cred, dh_params);

  s->session = initialize_tls_session ();
  gnutls_transport_set_ptr2 (s->session,
			     (gnutls_transport_ptr) s->strin,
			     (gnutls_transport_ptr) s->strout);
  gnutls_transport_set_pull_function (s->session, _tls_stream_pull);
  gnutls_transport_set_push_function (s->session, _tls_stream_push);
  
  rc = gnutls_handshake (s->session);
  if (rc < 0)
    {
      gnutls_deinit (s->session);
      s->last_err = rc;
      return EIO;
    }
  s->state = state_open;
  return 0;
}

static int
prepare_client_session (struct _tls_stream *s)
{
  int rc;
  static int protocol_priority[] = {GNUTLS_TLS1, GNUTLS_SSL3, 0};
  static int kx_priority[] = {GNUTLS_KX_RSA, 0};
  static int cipher_priority[] = {GNUTLS_CIPHER_3DES_CBC,
				  GNUTLS_CIPHER_ARCFOUR_128,
				  0};
  static int comp_priority[] = {GNUTLS_COMP_NULL, 0};
  static int mac_priority[] = {GNUTLS_MAC_SHA, GNUTLS_MAC_MD5, 0};

  gnutls_init (&s->session, GNUTLS_CLIENT);
  gnutls_protocol_set_priority (s->session, protocol_priority);
  gnutls_cipher_set_priority (s->session, cipher_priority);
  gnutls_compression_set_priority (s->session, comp_priority);
  gnutls_kx_set_priority (s->session, kx_priority);
  gnutls_mac_set_priority (s->session, mac_priority);

  gnutls_certificate_allocate_credentials (&x509_cred);
  if (ssl_cafile)
    {
      rc = gnutls_certificate_set_x509_trust_file (x509_cred,
						   ssl_cafile,
						   GNUTLS_X509_FMT_PEM);
      if (rc < 0)
	{
	  s->last_err = rc;
	  return -1;
	}
    }

  gnutls_credentials_set (s->session, GNUTLS_CRD_CERTIFICATE, x509_cred);

  gnutls_transport_set_ptr2 (s->session,
			     (gnutls_transport_ptr) s->strin,
			     (gnutls_transport_ptr) s->strout);
  gnutls_transport_set_pull_function (s->session, _tls_stream_pull);
  gnutls_transport_set_push_function (s->session, _tls_stream_push);
      
  return 0;
}
  
static int
_tls_open_client (mu_stream_t stream)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  int rc = 0;
  
  switch (s->state)
    {
    case state_closed:
      gnutls_certificate_free_credentials (x509_cred);
      if (s->session)
	gnutls_deinit (s->session);
      /* FALLTHROUGH */
      
    case state_init:
      prepare_client_session (s);
      rc = gnutls_handshake (s->session);
      if (rc < 0)
	{
	  s->last_err = rc;
	  gnutls_deinit (s->session);
	  s->state = state_init;
	  return -1;
	}
      break;

    default:
      return -1;
    }

  /* FIXME: if (ssl_cafile) verify_certificate (s->session); */
  s->state = state_open;
  return 0;
}

int
_tls_strerror (mu_stream_t stream, const char **pstr)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  *pstr = gnutls_strerror (s->last_err);
  return 0;
}

int
_tls_get_transport2 (mu_stream_t stream,
		     mu_transport_t *pin, mu_transport_t *pout)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  *pin = (mu_transport_t) s->strin;
  *pout = (mu_transport_t) s->strout;
  return 0;
}

int
_tls_wait (mu_stream_t stream, int *pflags, struct timeval *tvp)
{
  struct _tls_stream *s = mu_stream_get_owner (stream);
  if ((*pflags & (MU_STREAM_READY_RD|MU_STREAM_READY_WR)) == (MU_STREAM_READY_RD|MU_STREAM_READY_WR))
    return EINVAL; /* Sorry, can't wait for both input and output. */
  if (*pflags & MU_STREAM_READY_RD)
    return mu_stream_wait (s->strin, pflags, tvp);
  if (*pflags & MU_STREAM_READY_WR)
    return mu_stream_wait (s->strout, pflags, tvp);
  return EINVAL;
}

/* FIXME: if strin == strout sequential reads may intefere with
   sequential writes (they would share stream->offset). This should
   be fixed either in stream.c or here. In particular, mu_tls_stream_create_client
   will malfunction */
int
mu_tls_stream_create (mu_stream_t *stream,
		   mu_stream_t strin, mu_stream_t strout, int flags)
{
  struct _tls_stream *s;
  int rc;

  if (stream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  s = calloc (1, sizeof (*s));
  if (s == NULL)
    return ENOMEM;

  s->strin = strin;
  s->strout = strout;

  rc = mu_stream_create (stream, flags|MU_STREAM_NO_CHECK, s);
  if (rc)
    {
      free (s);
      return rc;
    }

  mu_stream_set_open (*stream, _tls_open, s);
  mu_stream_set_close (*stream, _tls_close, s);
  mu_stream_set_read (*stream, _tls_read, s);
  mu_stream_set_readline (*stream, _tls_readline, s);
  mu_stream_set_write (*stream, _tls_write, s);
  mu_stream_set_flush (*stream, _tls_flush, s);
  mu_stream_set_destroy (*stream, _tls_destroy, s);
  mu_stream_set_strerror (*stream, _tls_strerror, s);
  mu_stream_set_get_transport2 (*stream, _tls_get_transport2, s);
  mu_stream_set_wait (*stream, _tls_wait, s);
  _auth_lb_create (&s->lb);
  
  s->state = state_init;
  return 0;
}

int
mu_tls_stream_create_client (mu_stream_t *stream,
			  mu_stream_t strin, mu_stream_t strout, int flags)
{
  struct _tls_stream *s;
  int rc;

  if (stream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  s = calloc (1, sizeof (*s));
  if (s == NULL)
    return ENOMEM;

  s->strin = strin;
  s->strout = strout;
  
  rc = mu_stream_create (stream, flags|MU_STREAM_NO_CHECK, s);
  if (rc)
    {
      free (s);
      return rc;
    }

  mu_stream_set_open (*stream, _tls_open_client, s);
  mu_stream_set_close (*stream, _tls_close, s);
  mu_stream_set_read (*stream, _tls_read, s);
  mu_stream_set_readline (*stream, _tls_readline, s);
  mu_stream_set_write (*stream, _tls_write, s);
  mu_stream_set_flush (*stream, _tls_flush, s);
  mu_stream_set_destroy (*stream, _tls_destroy, s);
  mu_stream_set_strerror (*stream, _tls_strerror, s);
  mu_stream_set_get_transport2 (*stream, _tls_get_transport2, s);
  mu_stream_set_wait (*stream, _tls_wait, s);
  _auth_lb_create (&s->lb);
  
  s->state = state_init;
  return 0;
}

int
mu_tls_stream_create_client_from_tcp (mu_stream_t *stream, mu_stream_t tcp_str,
				   int flags)
{
  return mu_tls_stream_create_client (stream, tcp_str, tcp_str, flags);
}

#endif /* WITH_TLS */

/* EOF */

