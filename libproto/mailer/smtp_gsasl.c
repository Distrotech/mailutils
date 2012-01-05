/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/nls.h>
#include <mailutils/wordsplit.h>
#include <mailutils/diag.h>
#include <mailutils/errno.h>
#include <mailutils/cctype.h>
#include <mailutils/list.h>
#include <mailutils/util.h>
#include <mailutils/smtp.h>
#include <mailutils/stream.h>
#include <mailutils/sys/smtp.h>
#include <mailutils/gsasl.h>
#include <gsasl.h>

static int
get_implemented_mechs (Gsasl *ctx, mu_list_t *plist)
{
  char *listmech;
  mu_list_t supp = NULL;
  int rc;
  struct mu_wordsplit ws;
  
  rc =  gsasl_server_mechlist (ctx, &listmech);
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      "cannot get list of available SASL mechanisms: %s",
		      gsasl_strerror (rc));
      return 1;
    }

  if (mu_wordsplit (listmech, &ws, MU_WRDSF_DEFFLAGS))
    {
      mu_error (_("cannot split line `%s': %s"), listmech,
		mu_wordsplit_strerror (&ws));
      rc = errno;
    }
  else
    {
      size_t i;
      
      rc = mu_list_create (&supp);
      if (rc == 0)
	{
	  mu_list_set_destroy_item (supp, mu_list_free_item);
	  for (i = 0; i < ws.ws_wordc; i++) 
	    mu_list_append (supp, ws.ws_wordv[i]);
	}
      ws.ws_wordc = 0;
      ws.ws_wordv = NULL;
      mu_wordsplit_free (&ws);
    }
  free (listmech);
  *plist = supp;
  return rc;
}

static int
_smtp_callback (Gsasl *ctx, Gsasl_session *sctx, Gsasl_property prop)
{
  int rc = GSASL_OK;
  mu_smtp_t smtp = gsasl_callback_hook_get (ctx);
  const char *p = NULL;
  
  switch (prop)
    {
    case GSASL_PASSWORD:
      if (mu_smtp_get_param (smtp, MU_SMTP_PARAM_PASSWORD, &p) == 0 && p)
	gsasl_property_set (sctx, prop, p);
      else
	rc = GSASL_NO_PASSWORD;
      break;

    case GSASL_AUTHID:
    case GSASL_ANONYMOUS_TOKEN:
      if (mu_smtp_get_param (smtp, MU_SMTP_PARAM_USERNAME, &p) == 0 && p)
	gsasl_property_set (sctx, prop, p);
      else if (prop == GSASL_AUTHID)
	rc = GSASL_NO_AUTHID;
      else
	rc = GSASL_NO_ANONYMOUS_TOKEN;
      break;

    case GSASL_AUTHZID:
      rc = GSASL_NO_AUTHZID;
      break;
	
    case GSASL_SERVICE:
      if (mu_smtp_get_param (smtp, MU_SMTP_PARAM_SERVICE, &p) || !p)
	p = "smtp";
      gsasl_property_set (sctx, prop, p);
      break;

    case GSASL_REALM:
      if ((mu_smtp_get_param (smtp, MU_SMTP_PARAM_REALM, &p) || !p) &&
	  (mu_smtp_get_param (smtp, MU_SMTP_PARAM_DOMAIN, &p) || !p))
	rc = GSASL_NO_HOSTNAME;
      else
	gsasl_property_set (sctx, prop, p);
	break;

    case GSASL_HOSTNAME:
      if ((mu_smtp_get_param (smtp, MU_SMTP_PARAM_HOST, &p) || !p) &&
	  mu_get_host_name ((char**)&p))
	{
	  rc = GSASL_NO_HOSTNAME;
	  break;
	}
      else
	gsasl_property_set (sctx, prop, p);
      break;
	
    default:
	rc = GSASL_NO_CALLBACK;
	mu_diag_output (MU_DIAG_NOTICE,
			"unsupported callback property %d", prop);
	break;
    }

    return rc;
}

static int
restore_and_return (mu_smtp_t smtp, mu_stream_t *str, int code)
{
  mu_stream_unref (str[0]);
  mu_stream_unref (str[1]);
  return code;
}

int
insert_gsasl_stream (mu_smtp_t smtp, Gsasl_session *sess_ctx)
{
  mu_stream_t stream[2], newstream[2];
  int rc;

  rc = _mu_smtp_get_streams (smtp, stream);
  if (rc)
    {
      mu_error ("%s failed: %s", "MU_IOCTL_GET_STREAM",
		mu_stream_strerror (smtp->carrier, rc));
      return MU_ERR_FAILURE;
    }

  rc = gsasl_encoder_stream (&newstream[0], stream[0], sess_ctx,
			     MU_STREAM_READ);
  if (rc)
    {
      mu_error ("%s failed: %s", "gsasl_encoder_stream",
		mu_strerror (rc));
      return restore_and_return (smtp, stream, MU_ERR_FAILURE);
    }
  
  rc = gsasl_decoder_stream (&newstream[1], stream[1], sess_ctx,
			     MU_STREAM_WRITE);
  if (rc)
    {
      mu_error ("%s failed: %s", "gsasl_decoder_stream",
		mu_strerror (rc));
      mu_stream_destroy (&newstream[0]);
      return restore_and_return (smtp, stream, MU_ERR_FAILURE);
    }
      
  mu_stream_flush (stream[1]);
  mu_stream_unref (stream[0]);
  mu_stream_unref (stream[1]);

  rc = _mu_smtp_set_streams (smtp, newstream);
  
  if (rc)
    {
      mu_error ("%s failed when it should not: %s",
		"MU_IOCTL_SET_STREAM",
		mu_stream_strerror (smtp->carrier, rc));
      abort ();
    }

  return 0;
}
  
static int
do_gsasl_auth (mu_smtp_t smtp, Gsasl *ctx, const char *mech)
{
  Gsasl_session *sess;
  int rc, status;
  char *output = NULL;

  rc = gsasl_client_start (ctx, mech, &sess);
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_ERROR, "SASL gsasl_client_start: %s",
		      gsasl_strerror (rc));
      return MU_ERR_FAILURE;
    }

  status = mu_smtp_write (smtp, "AUTH %s\r\n", mech);
  MU_SMTP_CHECK_ERROR (smtp, rc);
  
  status = mu_smtp_response (smtp);
  MU_SMTP_CHECK_ERROR (smtp, status);
  if (smtp->replcode[0] != '3')
    {
      mu_diag_output (MU_DIAG_ERROR,
		      "GSASL handshake aborted: "
		      "unexpected reply: %s %s",
		      smtp->replcode, smtp->replptr);
      return MU_ERR_REPLY;
    }

  do
    {
      rc = gsasl_step64 (sess, smtp->replptr, &output);
      if (rc != GSASL_NEEDS_MORE && rc != GSASL_OK)
	break;

      status = mu_smtp_write (smtp, "%s\r\n", output);
      MU_SMTP_CHECK_ERROR (smtp, status);

      free (output);
      output = NULL;
      
      status = mu_smtp_response (smtp);
      MU_SMTP_CHECK_ERROR (smtp, status);
      if (smtp->replcode[0] == '2')
	{
	  rc = GSASL_OK;
	  break;
	}
      else if (smtp->replcode[0] != '3')
	break;
    } while (rc == GSASL_NEEDS_MORE);

  if (output)
    free (output);
  
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_ERROR, "GSASL error: %s", gsasl_strerror (rc));
      return 1;
    }

  if (smtp->replcode[0] != '2')
    {
      mu_diag_output (MU_DIAG_ERROR,
		      "GSASL handshake failed: %s %s",
		      smtp->replcode, smtp->replptr);
      return MU_ERR_REPLY;
    }

  /* Authentication successful */
  MU_SMTP_FSET (smtp, _MU_SMTP_AUTH);
  
  return insert_gsasl_stream (smtp, sess);
}

int
_mu_smtp_gsasl_auth (mu_smtp_t smtp)
{
  int rc;
  Gsasl *ctx;
  mu_list_t mech_list;
  const char *mech;
  
  rc = gsasl_init (&ctx);
  if (rc != GSASL_OK)
    {
      mu_diag_output (MU_DIAG_ERROR,
		      "cannot initialize GSASL: %s",
		      gsasl_strerror (rc));
      return MU_ERR_FAILURE;
    }
  rc = get_implemented_mechs (ctx, &mech_list);
  if (rc)
    return rc;
  rc = _mu_smtp_mech_impl (smtp, mech_list);
  if (rc)
    {
      mu_list_destroy (&mech_list);
      return rc;
    }

  rc = mu_smtp_mech_select (smtp, &mech);
  if (rc)
    {
      mu_diag_output (MU_DIAG_DEBUG,
		      "no suitable authentication mechanism found");
      return rc;
    }
  
  mu_diag_output (MU_DIAG_DEBUG, "selected authentication mechanism %s",
		  mech);
  
  gsasl_callback_hook_set (ctx, smtp);
  gsasl_callback_set (ctx, _smtp_callback);

  rc = do_gsasl_auth (smtp, ctx, mech);
  if (rc == 0)
    {
      /* Invalidate the capability list */
      mu_list_destroy (&smtp->capa);
    }
  
  return rc;
}
