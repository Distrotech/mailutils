/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* sieve callback implementations */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/address.h>
#include <mailutils/envelope.h>
#include <mailutils/errno.h>
#include <mailutils/header.h>

#include "sv.h"

/** mailutils errno to sieve error code translation. */

/*
   Note:

   The sieve engine doesn't really care what you return from the callbacks, it's
   either SIEVE_OK (== 0) or an error. This has the effect that if getheader()
   fails due to, for example, an IMAP protocol error, it will be assumed the
   header isn't present, which isn't true.

   On the other hand, it makes this really simple mapping below possible.
 */

static int 
sieve_err (int rc)
{
  return rc;
}

/** message query callbacks **/

int
sv_getsize (void *mc, int *size)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  size_t sz = 0;

  m->rc = message_size (m->msg, &sz);

  if (!m->rc)
    {
      *size = sz;

      mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY,
		      "msg uid %d getsize: %d\n", m->uid, *size);
    }

  return sieve_err (m->rc);
}

/*
   A given header can occur multiple times, so we return a pointer to a null
   terminated array of pointers to the values found for the named header.
 */
int
sv_getheader (void *mc, const char *name, const char ***body)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->rc = 0;

  if (!m->cache_filled)
    {
      header_t h = 0;
      size_t i = 0;
      char *fn = 0;
      char *fv = 0;

      m->cache_filled = 1;

      message_get_header (m->msg, &h);

      header_get_field_count (h, &i);

      mu_debug_print (m->debug, SV_DEBUG_HDR_FILL,
		      "msg uid %d cache: filling with %d fields\n",
		      m->uid, i);

      for (; i > 0; i--)
	{
	  m->rc = header_aget_field_name (h, i, &fn);
	  if (m->rc)
	    break;
	  m->rc = header_aget_field_value (h, i, &fv);
	  if (m->rc)
	    break;

	  mu_debug_print (m->debug, SV_DEBUG_HDR_FILL,
			  "msg uid %d cache: %s: %s\n", m->uid, fn, fv);

	  m->rc = sv_field_cache_add (&m->cache, fn, fv);

	  if (m->rc == 0)
	    {
	      fv = 0;		/* owned by the cache */
	    }
	  else
	    break;

	  /* the cache doesn't want it, and we don't need it */
	  free (fn);
	  fn = 0;
	}
      free (fn);
      free (fv);
      if (m->rc)
	{
	  mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY,
			  "msg uid %d cache: failed %s\n",
			  m->uid, sv_strerror (m->rc));
	  return sieve_err (m->rc);
	}
    }

  if ((m->rc = sv_field_cache_get (&m->cache, name, body)) == 0)
    {
      const char **b = *body;
      int i;
      for (i = 0; b[i]; i++)
	{
	  mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY,
			  "msg uid %d getheader: %s: %s\n", m->uid, name,
			  b[i]);
	}
    }
  else
    mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY,
		    "msg uid %d getheader: %s (not found)\n", m->uid, name);

  return sieve_err (m->rc);
}

/* We need to do a little more work to deduce the envelope addresses, they
   aren't necessarily stored in the mailbox. */
int
sv_getenvelope (void *m, const char *name, const char ***body)
{
  if (!name || strcasecmp (name, "from") == 0)
    {
      sv_msg_ctx_t *mc = m;
      envelope_t env = NULL;
      char buffer[128];
      address_t addr;
      const char **tmp;
      
      message_get_envelope (mc->msg, &env);
      if (envelope_sender (env, buffer, sizeof (buffer), NULL)
	  || address_create (&addr, buffer))
	return SIEVE_RUN_ERROR;

      if (address_get_email (addr, 1, buffer, sizeof (buffer), NULL))
	{
	  address_destroy (&addr);
	  return SIEVE_RUN_ERROR;
	}
      address_destroy (&addr);

      tmp = calloc (2, sizeof (tmp[0]));
      if (!tmp
	  || !(tmp[0] = strdup (buffer)))
	return SIEVE_NOMEM;
      tmp[1] = NULL;
      *body = tmp;
      return SIEVE_OK;
    }
  return SIEVE_RUN_ERROR;
}

/** message action callbacks */

/*
   The actions arguments are mostly callback data provided during the
   setup of the intepreter object, script object, and the execution of
   a script.

   The args are:

   void* ac; // action context, the member of the union Action.u associated
   // with this kind of action.
   void* ic, // from sieve_interp_alloc(, ic);
   void* sc, // from sieve_script_parse(, , sc, );
   void* mc, // from sieve_execute_script(, mc);
   const char** errmsg // you can fill it in if you return failure
 */

static void
action_log (sv_msg_ctx_t * mc, const char *action, const char *fmt,...)
{
  const char *script = mc->sc->file;
  message_t msg = mc->msg;
  sv_action_log_t log = mc->sc->ic->action_log;
  va_list ap;

  va_start (ap, fmt);

  if (log)
    log (script, msg, action, fmt, ap);

  va_end (ap);
}

int
sv_keep (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  //sieve_keep_context_t * a = (sieve_keep_context_t *) ac;
  m->rc = 0;

  action_log (mc, "KEEP", "");

  m->rc = sv_mu_mark_deleted (m->msg, 0);

  return sieve_err (m->rc);
}

int
sv_fileinto (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sieve_fileinto_context_t *a = (sieve_fileinto_context_t *) ac;
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->rc = 0;

  action_log (mc, "FILEINTO", "delivering into %s", a->mailbox);

  if ((m->svflags & SV_FLAG_NO_ACTIONS) == 0)
    {
      m->rc = message_save_to_mailbox (m->msg, m->ticket, m->debug, a->mailbox);
      if (!m->rc)
	{
	  m->rc = sv_mu_mark_deleted (m->msg, 1);
	}
    }

  return sieve_err (m->rc);
}

int
sv_redirect (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  sieve_redirect_context_t *a = (sieve_redirect_context_t *) ac;
  char *fromaddr = 0;
  address_t to = 0;
  address_t from = 0;

  action_log (mc, "REDIRECT", "to %s", a->addr);

  if (!m->mailer)
    {
      m->rc = ENOSYS;
      mu_debug_print (m->debug, MU_DEBUG_ERROR, "%s\n",
		      "redirect - requires a mailer");
      return SIEVE_FAIL;
    }

  if ((m->rc = address_create (&to, a->addr)))
    {
      mu_debug_print (m->debug, MU_DEBUG_ERROR,
		      "redirect - parsing to '%s' failed: [%d] %s\n",
		      a->addr, m->rc, mu_errstring (m->rc));
      goto end;
    }

  {
    envelope_t envelope = 0;
    size_t sz = 0;
    if ((m->rc = message_get_envelope (m->msg, &envelope)))
      goto end;

    if ((m->rc = envelope_sender (envelope, NULL, 0, &sz)))
      goto end;

    if (!(fromaddr = malloc (sz + 1)))
      {
	m->rc = ENOMEM;
	goto end;
      }
    if ((m->rc = envelope_sender (envelope, fromaddr, sz + 1, NULL)))
      goto end;
  }

  if ((m->rc = address_create (&from, fromaddr)))
    {
      mu_debug_print (m->debug, MU_DEBUG_ERROR,
		      "redirect - parsing from '%s' failed: [%d] %s\n",
		      a->addr, m->rc, mu_errstring (m->rc));
      goto end;
    }

  if ((m->rc = mailer_open (m->mailer, 0)))
    {
      mu_debug_print (m->debug, MU_DEBUG_ERROR,
		      "redirect - opening mailer '%s' failed: [%d] %s\n",
		      m->mailer, m->rc, mu_errstring (m->rc));
      goto end;
    }

  if ((m->rc = mailer_send_message (m->mailer, m->msg, from, to)))
    {
      mu_debug_print (m->debug, MU_DEBUG_ERROR,
		      "redirect - send from '%s' to '%s' failed: [%d] %s\n",
		      fromaddr, a->addr, m->rc, mu_errstring (m->rc));
      goto end;
    }

end:

  if (fromaddr)
    free (fromaddr);
  mailer_close (m->mailer);
  address_destroy (&to);
  address_destroy (&from);

  return sieve_err (m->rc);
}

int
sv_discard (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->rc = 0;

  action_log (mc, "DISCARD", "marking as deleted");

  if ((m->svflags & SV_FLAG_NO_ACTIONS) == 0)
    {
      m->rc = sv_mu_mark_deleted (m->msg, 1);
      if (m->rc)
	mu_debug_print (m->debug, MU_DEBUG_ERROR,
			"discard - deleting failed: [%d] %s\n",
			m->rc, mu_errstring (m->rc));
    }

  return sieve_err (m->rc);
}

int
sv_reject (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->rc = ENOSYS;

  action_log (mc, "REJECT", "");

  return sieve_err (m->rc);
}

#if 0
int
sv_notify (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_print_action ("NOTIFY", ac, ic, sc, mc);

  return SIEVE_OK;
}
#endif

#if 0
int
sv_autorespond (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  return SIEVE_FAIL;
}

int
sv_send_response (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  return SIEVE_FAIL;
}

sieve_vacation_t vacation =
{
  0,				/* min response */
  0,				/* max response */
  &sv_autorespond,		/* autorespond() */
  &sv_send_response		/* send_response() */
};

char *markflags[] =
{"\\flagged"};
sieve_imapflags_t mark =
{markflags, 1};
#endif

/* sieve error callbacks */

int
sv_parse_error (int lineno, const char *msg, void *ic, void *sc)
{
  sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;
  sv_script_ctx_t *s = (sv_script_ctx_t *) sc;

  if (i->parse_error)
    i->parse_error (s->file, lineno, msg);

  return SIEVE_OK;
}

int
sv_execute_error (const char *msg, void *ic, void *sc, void *mc)
{
  sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;
  sv_script_ctx_t *s = (sv_script_ctx_t *) sc;
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  if (i->execute_error)
    i->execute_error (s->file, m->msg, m->rc, msg);

  return SIEVE_OK;
}

/* register supported callbacks */

int
sv_register_callbacks (sieve_interp_t * i)
{
  int rc = 0;

  /* These 4 callbacks are mandatory. */
  if (!rc)
    rc = sieve_register_size (i, &sv_getsize);
  if (!rc)
    rc = sieve_register_header (i, &sv_getheader);
  if (!rc)
    rc = sieve_register_redirect (i, &sv_redirect);
  if (!rc)
    rc = sieve_register_keep (i, &sv_keep);

  if (!rc)
    rc = sieve_register_envelope (i, &sv_getenvelope);

  if (!rc)
    rc = sieve_register_discard (i, &sv_discard);

#if 0
  if (!rc)
    rc = sieve_register_reject (i, &sv_reject);
#endif
  /* The "fileinto" extension. */
  if (!rc)
    rc = sieve_register_fileinto (i, &sv_fileinto);
#if 0
  if (!rc)
    rc = sieve_register_vacation (i, &sv_vacation);
#endif
#if 0
  if (!rc)
    rc = sieve_register_imapflags (i, &mark);
#endif
#if 0
  if (!rc)
    rc = sieve_register_notify (i, &sv_notify);
#endif
  if (!rc)
    rc = sieve_register_parse_error (i, &sv_parse_error);
  if (!rc)
    rc = sieve_register_execute_error (i, &sv_execute_error);
  return rc;
}
