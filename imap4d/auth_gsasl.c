/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include "imap4d.h"
#include <gsasl.h>
#include <mailutils/gsasl.h>

static Gsasl_ctx *ctx;   
static Gsasl_session_ctx *sess_ctx; 

static int
create_gsasl_stream (stream_t *newstr, stream_t str, int flags)
{
  int rc;
  
  rc = gsasl_stream_create (newstr, str, sess_ctx, flags);
  if (rc)
    {
      syslog (LOG_ERR, _("cannot create SASL stream: %s"),
	      mu_strerror (rc));
      return RESP_NO;
    }

  if ((rc = stream_open (*newstr)) != 0)
    {
      char *p;
      if (stream_strerror (*newstr, &p))
	p = mu_strerror (rc);
      syslog (LOG_ERR, _("cannot open SASL input stream: %s"), p);
      return RESP_NO;
    }

  return RESP_OK;
}

void
gsasl_replace_streams (void *self, void *data)
{
  stream_t *s = data;

  util_set_input (s[0]);
  util_set_output (s[1]);
  free (s);
  util_event_remove (self);
  free (self);
  return 0;
}


static int
auth_gsasl (struct imap4d_command *command,
	     char *auth_type, char *arg, char **username)
{
  char *input = NULL;
  char output[512];
  size_t output_len;
  char *s;
  int rc;

  input = util_getword (arg, &s);
  util_unquote (&input);
  
  rc = gsasl_server_start (ctx, auth_type, &sess_ctx);
  if (rc != GSASL_OK)
    {
      syslog (LOG_NOTICE, _("SASL gsasl_server_start: %s"),
	      gsasl_strerror(rc));
      return 0;
    }

  gsasl_server_application_data_set (sess_ctx, username);

  output[0] = '\0';
  output_len = sizeof (output);

  while ((rc = gsasl_server_step_base64 (sess_ctx, input, output, output_len))
	 == GSASL_NEEDS_MORE)
    {
      util_send ("+ %s\r\n", output);
      input = imap4d_readline_ex ();
    }
  while (rc == GSASL_NEEDS_MORE);

  if (rc != GSASL_OK)
    {
      syslog (LOG_NOTICE, _("GSASL error: %s"), gsasl_strerror (rc));
      return RESP_NO;
    }

  if (*username == NULL)
    {
      syslog (LOG_NOTICE, _("GSASL %s: cannot get username"), auth_type);
      return RESP_NO;
    }

  if (sess_ctx)
    {
      stream_t in, out, new_in, new_out;
      stream_t *s;
      int infd, outfd;
      
      util_get_input (&in);
      stream_get_fd (in, &infd);
      
      util_get_output (&out);
      stream_get_fd (out, &outfd);
      if (create_gsasl_stream (&new_in, infd, MU_STREAM_READ))
	return RESP_NO;
      if (create_gsasl_stream (&new_out, outfd, MU_STREAM_WRITE))
	{
	  stream_destroy (&new_in, stream_get_owner (new_in));
	  return RESP_NO;
	}

      s = calloc (2, sizeof (stream_t));
      s[0] = new_in;
      s[1] = new_out;
      util_register_event (STATE_NONAUTH, STATE_AUTH,
			   gsasl_replace_streams, s);
    }

  
  auth_gsasl_capa_init (1);
  return RESP_OK;
}

static void
auth_gsasl_capa_init (int disable)
{
  int rc;
  char *listmech, *name, *s;
  size_t size;

  rc = gsasl_server_listmech (ctx, NULL, &size);
  if (rc != GSASL_OK)
    return;

  listmech = malloc (size);
  if (!listmech)
    imap4d_bye (ERR_NO_MEM);
  
  rc = gsasl_server_listmech (ctx, listmech, &size);
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
  char **username = gsasl_server_application_data_get (ctx);
  *username = strdup (authentication_id ?
		      authentication_id : authorization_id);
  return GSASL_OK;
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

  if (username && authentication_id)
    *username = strdup (authentication_id);

  return gsasl_md5pwd_get_password (gsasl_cram_md5_pwd, authentication_id,
				    key, keylen);
}

void
auth_gsasl_init ()
{
  int rc;
  
  rc = gsasl_init (&ctx);
  if (rc != GSASL_OK)
    {
      syslog (LOG_NOTICE, _("cannot initialize libgsasl: %s"),
	      gsasl_strerror (rc));
    }

  gsasl_server_callback_realm_set (ctx, cb_realm);
  gsasl_server_callback_external_set (ctx, cb_external);
  gsasl_server_callback_validate_set (ctx, cb_validate);
  gsasl_server_callback_service_set (ctx, cb_service);

  if (gsasl_cram_md5_pwd && access (gsasl_cram_md5_pwd, R_OK) == 0)
    {
      gsasl_server_callback_retrieve_set (ctx, cb_retrieve);
    }
  
  auth_gsasl_capa_init (0);
}

