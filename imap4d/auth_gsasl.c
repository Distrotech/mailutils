/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005, 2007 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"
#include <gsasl.h>
#include <mailutils/gsasl.h>
#ifdef USE_SQL
# include <mailutils/sql.h>
#endif

static Gsasl_ctx *ctx;   
static Gsasl_session_ctx *sess_ctx; 

static void auth_gsasl_capa_init (int disable);

static int
create_gsasl_stream (mu_stream_t *newstr, mu_stream_t transport, int flags)
{
  int rc;
  
  rc = mu_gsasl_stream_create (newstr, transport, sess_ctx, flags);
  if (rc)
    {
      mu_diag_output (MU_DIAG_ERROR, _("cannot create SASL stream: %s"),
	      mu_strerror (rc));
      return RESP_NO;
    }

  if ((rc = mu_stream_open (*newstr)) != 0)
    {
      const char *p;
      if (mu_stream_strerror (*newstr, &p))
	p = mu_strerror (rc);
      mu_diag_output (MU_DIAG_ERROR, _("cannot open SASL input stream: %s"), p);
      return RESP_NO;
    }

  return RESP_OK;
}

int
gsasl_replace_streams (void *self, void *data)
{
  mu_stream_t *s = data;

  util_set_input (s[0]);
  util_set_output (s[1]);
  free (s);
  util_event_remove (self);
  free (self);
  return 0;
}

static void
finish_session (void)
{
  gsasl_server_finish (sess_ctx);
}

static int
auth_gsasl (struct imap4d_command *command,
	    char *auth_type, char *arg, char **username)
{
  char *input = NULL;
  char *output;
  char *s;
  int rc;

  input = util_getword (arg, &s);
  util_unquote (&input);
  
  rc = gsasl_server_start (ctx, auth_type, &sess_ctx);
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("SASL gsasl_server_start: %s"),
	      gsasl_strerror(rc));
      return 0;
    }

  gsasl_server_application_data_set (sess_ctx, username);

  output = NULL;
  while ((rc = gsasl_step64 (sess_ctx, input, &output)) == GSASL_NEEDS_MORE)
    {
      util_send ("+ %s\r\n", output);
      input = imap4d_readline_ex ();
    }
  
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("GSASL error: %s"), gsasl_strerror (rc));
      free (output);
      return RESP_NO;
    }

  /* Some SASL mechanisms output data when GSASL_OK is returned */
  if (output[0])
    util_send ("+ %s\r\n", output);
  
  free (output);

  if (*username == NULL)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("GSASL %s: cannot get username"), auth_type);
      return RESP_NO;
    }

  if (sess_ctx)
    {
      mu_stream_t tmp, new_in, new_out;
      mu_stream_t *s;

      util_get_input (&tmp);
      if (create_gsasl_stream (&new_in, tmp, MU_STREAM_READ))
	return RESP_NO;
      util_get_output (&tmp);
      if (create_gsasl_stream (&new_out, tmp, MU_STREAM_WRITE))
	{
	  mu_stream_destroy (&new_in, mu_stream_get_owner (new_in));
	  return RESP_NO;
	}

      s = calloc (2, sizeof (mu_stream_t));
      s[0] = new_in;
      s[1] = new_out;
      util_register_event (STATE_NONAUTH, STATE_AUTH,
			   gsasl_replace_streams, s);
      util_atexit (finish_session);
    }
  
  auth_gsasl_capa_init (1);
  return RESP_OK;
}

static void
auth_gsasl_capa_init (int disable)
{
  int rc;
  char *listmech, *name, *s;

  rc =  gsasl_server_mechlist (ctx, &listmech);
  if (rc != GSASL_OK)
    return;

  for (name = strtok_r (listmech, " ", &s); name;
       name = strtok_r (NULL, " ", &s))
    {
      if (disable)
	auth_remove (name);
      else
	auth_add (strdup (name), auth_gsasl);
    }
      
  free (listmech);
}

/* This is for DIGEST-MD5 */
static int
cb_realm (Gsasl_session_ctx *ctx, char *out, size_t *outlen, size_t nth)
{
  char *realm = util_localname ();

  if (nth > 0)
    return GSASL_NO_MORE_REALMS;

  if (out)
    {
      if (*outlen < strlen (realm))
	return GSASL_TOO_SMALL_BUFFER;
      memcpy (out, realm, strlen (realm));
    }

  *outlen = strlen (realm);

  return GSASL_OK;
}

static int
cb_validate (Gsasl_session_ctx *ctx,
	     const char *authorization_id,
	     const char *authentication_id,
	     const char *password)
{
  int rc;
  struct mu_auth_data *auth;
  char **username = gsasl_server_application_data_get (ctx);

  *username = strdup (authentication_id ?
		      authentication_id : authorization_id);
  
  auth_data = mu_get_auth_by_name (*username);

  if (auth_data == NULL)
    return GSASL_AUTHENTICATION_ERROR;

  rc = mu_authenticate (auth, password);
  mu_auth_data_free (auth);
  
  return rc == 0 ? GSASL_OK : GSASL_AUTHENTICATION_ERROR;
}

#define GSSAPI_SERVICE "imap"

static int
cb_service (Gsasl_session_ctx *ctx, char *srv, size_t *srvlen,
	    char *host, size_t *hostlen)
{
  char *hostname = util_localname ();

  if (srv)
    {
      if (*srvlen < strlen (GSSAPI_SERVICE))
       return GSASL_TOO_SMALL_BUFFER;

      memcpy (srv, GSSAPI_SERVICE, strlen (GSSAPI_SERVICE));
    }

  if (srvlen)
    *srvlen = strlen (GSSAPI_SERVICE);

  if (host)
    {
      if (*hostlen < strlen (hostname))
       return GSASL_TOO_SMALL_BUFFER;

      memcpy (host, hostname, strlen (hostname));
    }

  if (hostlen)
    *hostlen = strlen (hostname);

  return GSASL_OK;
}

/* This gets called when SASL mechanism EXTERNAL is invoked */
static int
cb_external (Gsasl_session_ctx *ctx)
{
  return GSASL_AUTHENTICATION_ERROR;
}

/* This gets called when SASL mechanism CRAM-MD5 or DIGEST-MD5 is invoked */

static int
cb_retrieve (Gsasl_session_ctx *ctx,
	     const char *authentication_id,
	     const char *authorization_id,
	     const char *realm,
	     char *key,
	     size_t *keylen)
{
  char **username = gsasl_server_application_data_get (ctx);

  if (username && *username == 0 && authentication_id)
    *username = strdup (authentication_id);

  if (mu_gsasl_module_data.cram_md5_pwd
      && access (mu_gsasl_module_data.cram_md5_pwd, R_OK) == 0)
    {
      int rc = gsasl_md5pwd_get_password (mu_gsasl_module_data.cram_md5_pwd,
					  authentication_id,
					  key, keylen);
      if (rc == GSASL_OK)
	return rc;
    }
  
#ifdef USE_SQL
  if (mu_sql_module_config.password_type == password_plaintext)
    {
      char *passwd;
      int status = mu_sql_getpass (username, &passwd);
      if (status == 0)
	{
	  *keylen = strlen (passwd);
	  if (key)
	    memcpy (key, passwd, *keylen);
	  free (passwd);
	  return GSASL_OK;
	}
    }
#endif
  
  return GSASL_AUTHENTICATION_ERROR; 
}

void
auth_gsasl_init ()
{
  int rc;

  rc = gsasl_init (&ctx);
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("cannot initialize libgsasl: %s"),
	      gsasl_strerror (rc));
    }

  gsasl_server_callback_realm_set (ctx, cb_realm);
  gsasl_server_callback_external_set (ctx, cb_external);
  gsasl_server_callback_validate_set (ctx, cb_validate);
  gsasl_server_callback_service_set (ctx, cb_service);

  gsasl_server_callback_retrieve_set (ctx, cb_retrieve);
  
  auth_gsasl_capa_init (0);
}

