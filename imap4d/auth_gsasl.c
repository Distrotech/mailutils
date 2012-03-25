/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2005, 2007-2012 Free Software Foundation, Inc.

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

#include "imap4d.h"
#include <gsasl.h>
#include <mailutils/gsasl.h>
#ifdef USE_SQL
# include <mailutils/sql.h>
#endif

static Gsasl *ctx;   
static Gsasl_session *sess_ctx; 

static void auth_gsasl_capa_init (int disable);

static void
finish_session (void)
{
  gsasl_finish (sess_ctx);
}

static int
restore_and_return (struct imap4d_auth *ap, mu_stream_t *str, int resp)
{
  mu_stream_unref (str[0]);
  mu_stream_unref (str[1]);
  ap->response = resp;
  return imap4d_auth_resp;
}

static enum imap4d_auth_result
auth_gsasl (struct imap4d_auth *ap)
{
  char *input_str = NULL;
  size_t input_size = 0;
  size_t input_len;
  char *output;
  int rc;
  
  rc = gsasl_server_start (ctx, ap->auth_type, &sess_ctx);
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("SASL gsasl_server_start: %s"),
 	              gsasl_strerror (rc));
      return imap4d_auth_fail;
    }

  gsasl_callback_hook_set (ctx, &ap->username);

  output = NULL;
  while ((rc = gsasl_step64 (sess_ctx, input_str, &output))
	   == GSASL_NEEDS_MORE)
    {
      io_sendf ("+ %s\n", output);
      io_getline (&input_str, &input_size, &input_len);
    }
  
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("GSASL error: %s"),
		      gsasl_strerror (rc));
      free (input_str);
      free (output);
      ap->response = RESP_NO;
      return imap4d_auth_resp;
    }

  /* Some SASL mechanisms output additional data when GSASL_OK is
     returned, and clients must respond with an empty response. */
  if (output[0])
    {
      io_sendf ("+ %s\n", output);
      io_getline (&input_str, &input_size, &input_len);
      if (input_len != 0)
	{
	  mu_diag_output (MU_DIAG_NOTICE, _("non-empty client response"));
          free (input_str);
          free (output);
	  ap->response = RESP_NO;
	  return imap4d_auth_resp;
	}
    }

  free (input_str);
  free (output);

  if (ap->username == NULL)
    {
      mu_diag_output (MU_DIAG_NOTICE, _("GSASL %s: cannot get username"),
		      ap->auth_type);
      ap->response = RESP_NO;
      return imap4d_auth_resp;
    }

  auth_gsasl_capa_init (1);
  if (sess_ctx)
    {
      mu_stream_t stream[2], newstream[2];

      rc = mu_stream_ioctl (iostream, MU_IOCTL_SUBSTREAM, MU_IOCTL_OP_GET, 
                            stream);
      if (rc)
	{
	  mu_error (_("%s failed: %s"), "MU_IOCTL_GET_STREAM",
		    mu_stream_strerror (iostream, rc));
	  ap->response = RESP_NO;
	  return imap4d_auth_resp;
	}
      rc = gsasl_encoder_stream (&newstream[0], stream[0], sess_ctx,
				 MU_STREAM_READ);
      if (rc)
	{
	  mu_error (_("%s failed: %s"), "gsasl_encoder_stream",
		    mu_strerror (rc));
	  return restore_and_return (ap, stream, RESP_NO);
	}

      rc = gsasl_decoder_stream (&newstream[1], stream[1], sess_ctx,
				 MU_STREAM_WRITE);
      if (rc)
	{
	  mu_error (_("%s failed: %s"), "gsasl_decoder_stream",
		    mu_strerror (rc));
	  mu_stream_destroy (&newstream[0]);
	  return restore_and_return (ap, stream, RESP_NO);
	}
      
      if (ap->username)
	{
	  if (imap4d_session_setup (ap->username))
	    {
	      mu_stream_destroy (&newstream[0]);
	      mu_stream_destroy (&newstream[1]);
	      return restore_and_return (ap, stream, RESP_NO);
	    }
	}

      /* FIXME: This is not reflected in the transcript. */
      io_stream_completion_response (stream[1], ap->command, RESP_OK,
				     "%s authentication successful",
				     ap->auth_type);
      mu_stream_flush (stream[1]);

      mu_stream_unref (stream[0]);
      mu_stream_unref (stream[1]);
      
      rc = mu_stream_ioctl (iostream, MU_IOCTL_SUBSTREAM, 
                            MU_IOCTL_OP_SET, newstream);
      if (rc)
	{
	  mu_error (_("%s failed when it should not: %s"),
		    "MU_IOCTL_SET_STREAM",
		    mu_stream_strerror (iostream, rc));
	  abort ();
	}

      mu_stream_unref (newstream[0]);
      mu_stream_unref (newstream[1]);
      
      util_atexit (finish_session);
      return imap4d_auth_ok;
    }
  
  ap->response = RESP_OK;
  return imap4d_auth_resp;
}

static void
auth_gsasl_capa_init (int disable)
{
  int rc;
  char *listmech;
  struct mu_wordsplit ws;
  
  rc =  gsasl_server_mechlist (ctx, &listmech);
  if (rc != GSASL_OK)
    return;

  ws.ws_delim = " ";
  if (mu_wordsplit (listmech, &ws,
		    MU_WRDSF_DELIM|MU_WRDSF_SQUEEZE_DELIMS|
		    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
    {
      mu_error (_("cannot split line `%s': %s"), listmech,
		mu_wordsplit_strerror (&ws));
    }
  else
    {
      size_t i;

      for (i = 0; i < ws.ws_wordc; i++)
	{
	  if (disable)
	    auth_remove (ws.ws_wordv[i]);
	  else
	    {
	      auth_add (ws.ws_wordv[i], auth_gsasl);
	      ws.ws_wordv[i] = NULL;
	    }
	}
      mu_wordsplit_free (&ws);
    }
  free (listmech);
}

#define IMAP_GSSAPI_SERVICE "imap"

static int
retrieve_password (Gsasl *ctx, Gsasl_session *sctx)
{
  char **username = gsasl_callback_hook_get (ctx);
  const char *authid = gsasl_property_get (sctx, GSASL_AUTHID);
  
  if (username && *username == 0)
    *username = mu_strdup (authid);

  if (mu_gsasl_module_data.cram_md5_pwd
      && access (mu_gsasl_module_data.cram_md5_pwd, R_OK) == 0)
    {
      char *key;
      int rc = gsasl_simple_getpass (mu_gsasl_module_data.cram_md5_pwd,
				     authid, &key);
      if (rc == GSASL_OK)
	{
	  gsasl_property_set (sctx, GSASL_PASSWORD, key);
	  free (key);
	  return rc;
	}
    }
  
#ifdef USE_SQL
  if (mu_sql_module_config.password_type == password_plaintext)
    {
      char *passwd;
      int status = mu_sql_getpass (*username, &passwd);
      if (status == 0)
	{
	  gsasl_property_set (sctx, GSASL_PASSWORD, passwd);
	  free (passwd);
	  return GSASL_OK;
	}
    }
#endif
  
  return GSASL_AUTHENTICATION_ERROR; 
}

static int
cb_validate (Gsasl *ctx, Gsasl_session *sctx)
{
  int rc;
  struct mu_auth_data *auth;
  char **username = gsasl_callback_hook_get (ctx);
  const char *authid = gsasl_property_get (sctx, GSASL_AUTHID);
  const char *pass = gsasl_property_get (sctx, GSASL_PASSWORD);

  if (!authid)
    return GSASL_NO_AUTHID;
  if (!pass)
    return GSASL_NO_PASSWORD;
  
  *username = mu_strdup (authid);
  
  auth = mu_get_auth_by_name (*username);

  if (auth == NULL)
    return GSASL_AUTHENTICATION_ERROR;

  rc = mu_authenticate (auth, pass);
  mu_auth_data_free (auth);
  
  return rc == 0 ? GSASL_OK : GSASL_AUTHENTICATION_ERROR;
}
  
static int
callback (Gsasl *ctx, Gsasl_session *sctx, Gsasl_property prop)
{
    int rc = GSASL_OK;

    switch (prop) {
    case GSASL_PASSWORD:
      rc = retrieve_password (ctx, sctx);
      break;
      
    case GSASL_SERVICE:
      gsasl_property_set (sctx, prop,
			  mu_gsasl_module_data.service ?
			    mu_gsasl_module_data.service :IMAP_GSSAPI_SERVICE);
      break;
      
    case GSASL_REALM:
      gsasl_property_set (sctx, prop,
			  mu_gsasl_module_data.realm ?
			    mu_gsasl_module_data.realm : util_localname ());
      break;
      
    case GSASL_HOSTNAME:
      gsasl_property_set (sctx, prop,
			  mu_gsasl_module_data.hostname ?
			    mu_gsasl_module_data.hostname : util_localname ());
      break;
      
#if 0
    FIXME:
    case GSASL_VALIDATE_EXTERNAL:
    case GSASL_VALIDATE_SECURID:
#endif
      
    case GSASL_VALIDATE_SIMPLE:
      rc = cb_validate (ctx, sctx);
      break;
      
    case GSASL_VALIDATE_ANONYMOUS:
      if (mu_gsasl_module_data.anon_user)
	{
	  char **username = gsasl_callback_hook_get (ctx);
	  mu_diag_output (MU_DIAG_INFO, _("anonymous user %s logged in"),
			  gsasl_property_get (sctx, GSASL_ANONYMOUS_TOKEN));
	  *username = mu_strdup (mu_gsasl_module_data.anon_user);
	}
      else
	{
	  mu_diag_output (MU_DIAG_ERR,
			  _("attempt to log in as anonymous user denied"));
	}
      break;
      
    case GSASL_VALIDATE_GSSAPI:
      {
	char **username = gsasl_callback_hook_get (ctx);
	*username = mu_strdup (gsasl_property_get(sctx, GSASL_AUTHZID));
	break;
      }
      
    default:
	rc = GSASL_NO_CALLBACK;
	mu_error (_("unsupported callback property %d"), prop);
	break;
    }

    return rc;
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

  gsasl_callback_set (ctx, callback);
  
  auth_gsasl_capa_init (0);
}

