/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mailutils/argp.h>
#include <mailutils/error.h>
#include <mailutils/mu_auth.h>
#include <mailutils/tls.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>

#ifdef WITH_TLS

#include <gnutls/gnutls.h>

#define DH_BITS 768

static gnutls_dh_params dh_params;
static gnutls_certificate_server_credentials x509_cred;
static char *ssl_cert = NULL;
static char *ssl_key = NULL;
static char *ssl_cafile = NULL;

#define ARG_SSL_CERT   1
#define ARG_SSL_KEY    2
#define ARG_SSL_CAFILE 3

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
  switch (key)
    {
    case ARG_SSL_CERT:
      ssl_cert = arg;
      break;

    case ARG_SSL_KEY:
      ssl_key = arg;
      break;

    case ARG_SSL_CAFILE:
      ssl_cafile = arg;
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

int
mu_check_tls_environment (void)
{
  /* Return: zero means NOT READY, one means READY */

  if (ssl_cert && ssl_key)
    {
      struct stat st;

      if (stat (ssl_cert, &st) == -1)
	return 0;
      if (!(st.st_mode & S_IFREG) || !(st.st_mode & S_IFLNK))
	{
	  mu_error (_("%s is not a regular file or a symbolic link."),
		    ssl_cert);
	  return 0;
	}

      if (stat (ssl_key, &st) == -1)
	return 0;
      if (!(st.st_mode & S_IFREG) || !(st.st_mode & S_IFLNK))
	{
	  mu_error (_("%s is not a regular file or a symbolic link."),
		    ssl_key);
	  return 0;
	}
      if ((st.st_mode & S_IRWXG) || (st.st_mode & S_IRWXO))
	{
	  mu_error (_("Wrong permissions on %s. Set 0600."), ssl_key);
	  return 0;
	}
    }
  else
    return 0;

  return 1;
}

int
mu_init_tls_libs (void)
{
  return !gnutls_global_init (); /* Returns 1 on success */
}

void
mu_deinit_tls_libs (void)
{
  gnutls_global_deinit ();
}

static void
generate_dh_params (void)
{
  gnutls_datum prime, generator;

  gnutls_dh_params_init (&dh_params);
  gnutls_dh_params_generate (&prime, &generator, DH_BITS);
  gnutls_dh_params_set (dh_params, prime, generator, DH_BITS);

  free (prime.data);
  free (generator.data);
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
  int ifd;
  int ofd;
  int last_err;
  enum tls_stream_state state;
  gnutls_session session;  
};


static void
_tls_destroy (stream_t stream)
{
  struct _tls_stream *s = stream_get_owner (stream);
  if (s->session && s->state == state_closed)
    {
      gnutls_deinit (s->session);
      s->state = state_destroyed;
    }
  free (s);
}
    
static int
_tls_read (stream_t stream, char *optr, size_t osize,
	   off_t offset, size_t *nbytes)
{
  struct _tls_stream *s = stream_get_owner (stream);
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
_tls_readline (stream_t stream, char *optr, size_t osize,
		off_t offset, size_t *nbytes)
{
  struct _tls_stream *s = stream_get_owner (stream);
  int rc;
  char *ptr;
  size_t rdsize;
  
  if (!stream || s->state != state_open || osize < 2)
    return EINVAL;

  osize--; /* Allow for terminating zero */
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

  ptr[rdsize] = 0;
  
  if (nbytes)
    *nbytes = rdsize;
  
  return 0;
}

static int
_tls_write (stream_t stream, const char *iptr, size_t isize,
	    off_t offset, size_t *nbytes)
{
  struct _tls_stream *s = stream_get_owner (stream);
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
_tls_flush (stream_t stream)
{
  /* noop */
  return 0;
}

static int
_tls_close (stream_t stream)
{
  struct _tls_stream *s = stream_get_owner (stream);
  if (s->session && s->state == state_open)
    {
      gnutls_bye (s->session, GNUTLS_SHUT_RDWR);
      s->state = state_closed;
    }
  return 0;
}

static int
_tls_open (stream_t stream)
{
  struct _tls_stream *s = stream_get_owner (stream);
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
  gnutls_transport_set_ptr2 (s->session, s->ifd, s->ofd);

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

int
_tls_strerror (stream_t stream, char **pstr)
{
  struct _tls_stream *s = stream_get_owner (stream);
  *pstr = gnutls_strerror (s->last_err);
  return 0;
}

/* FIXME: It returns only input fd */
int
_tls_get_fd (stream_t stream, int *pfd)
{
  struct _tls_stream *s = stream_get_owner (stream);
  *pfd = s->ifd;
  return 0;
}

int
tls_stream_create (stream_t *stream, int in_fd, int out_fd, int flags)
{
  struct _tls_stream *s;
  int rc;

  if (stream == NULL)
    return EINVAL;

  s = calloc (1, sizeof (*s));
  if (s == NULL)
    return ENOMEM;

  s->ifd = in_fd;
  s->ofd = out_fd;

  rc = stream_create (stream, flags|MU_STREAM_NO_CHECK, s);
  if (rc)
    {
      free (s);
      return rc;
    }

  stream_set_open (*stream, _tls_open, s);
  stream_set_close (*stream, _tls_close, s);
  stream_set_read (*stream, _tls_read, s);
  stream_set_readline (*stream, _tls_readline, s);
  stream_set_write (*stream, _tls_write, s);
  stream_set_flush (*stream, _tls_flush, s);
  stream_set_destroy (*stream, _tls_destroy, s);
  stream_set_strerror (*stream, _tls_strerror, s);
  stream_set_fd (*stream, _tls_get_fd, s);
  
  s->state = state_init;
  return 0;
}

#endif /* WITH_TLS */

/* EOF */
