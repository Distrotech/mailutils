/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mailutils/argp.h>
#include <mailutils/error.h>
#include <mailutils/mu_auth.h>
#include <mailutils/tls.h>
#include <mailutils/nls.h>

#ifdef WITH_TLS

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
  {"ssl-key", ARG_SSL_KEY, N_("KEY"), 0,
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
      if ((st.st_mode & S_IRWXG) || (st.st_mode & S_IRWXO))
	{
	  mu_error (_("Wrong permissions on %s. Set 0600."), ssl_cert);
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
  int rs = gnutls_global_init ();

  if (rs == 0)			/* Reverse for tls_available */
    rs = 1;
  else
    rs = 0;

  return rs;			/* Returns 1 on success */
}

int
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

  return (gnutls_session) session;
}

gnutls_session
mu_init_tls_server (int fd_in, int fd_out)
{
  int rs = 0;
  gnutls_session session = 0;

  gnutls_certificate_allocate_credentials (&x509_cred);

  if (ssl_cafile)
    gnutls_certificate_set_x509_trust_file (x509_cred, ssl_cafile,
					    GNUTLS_X509_FMT_PEM);

  rs = gnutls_certificate_set_x509_key_file (x509_cred,
					     ssl_cert, ssl_key,
					     GNUTLS_X509_FMT_PEM);
  if (rs < 0)
    {
      mu_error (_("cannot parse certificate/key: %s"), gnutls_strerror (rs));
      return 0;
    }
  
  generate_dh_params ();
  gnutls_certificate_set_dh_params (x509_cred, dh_params);

  session = initialize_tls_session ();
  gnutls_transport_set_ptr2 (session, fd_in, fd_out);

  rs = gnutls_handshake (session);
  if (rs < 0)
    {
      gnutls_deinit (session);
      mu_error (_("TLS/SSL handshake failed: %s"), gnutls_strerror (rs));
      return 0;			/* failed */
    }
  return (gnutls_session) session;
}

void
mu_deinit_tls_server (gnutls_session session)
{
  if (session)
    {
      gnutls_bye (session, GNUTLS_SHUT_RDWR);
      gnutls_deinit (session);
    }
}

#endif /* WITH_TLS */

/* EOF */
