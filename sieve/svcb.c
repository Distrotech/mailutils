/* sieve callback implementations */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sv.h"

/** message query callbacks **/

int
sv_getsize (void *mc, int *size)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  size_t sz = 0;

  m->rc = message_size (m->msg, &sz);

  if(m->rc)
    return SIEVE_RUN_ERROR;

  *size = sz;

  mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY, "msg uid %d getsize: %d\n",
      m->uid, size);

  return SIEVE_OK;
}

/*
A given header can occur multiple times, so we return a pointer
to a null terminated array of pointers to the values found for
the named header.
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
			  "msg uid %d cache: %s: %s\n", m->uid,
			  fn, fv);

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
	  if (m->rc == ENOMEM)
	    return SIEVE_NOMEM;
	  return SIEVE_RUN_ERROR;
	}
    }

  if ((m->rc = sv_field_cache_get (&m->cache, name, body)) == EOK)
    {
      const char **b = *body;
      int i;
      for (i = 0; b[i]; i++)
	{
	  mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY, "msg uid %d getheader: %s: %s\n",
		    m->uid, name, b[i]);
	}
    }
  else
    mu_debug_print (m->debug, SV_DEBUG_MSG_QUERY, "msg uid %d getheader: %s (not found)\n",
		    m->uid, name);

  switch (m->rc)
    {
    case 0:
      return SIEVE_OK;
    case ENOMEM:
      return SIEVE_NOMEM;
    }
  return SIEVE_RUN_ERROR;
}

/*
name will always be "to" or "from"

envelope_t doesn't seem to allow "to" to be gotten, just "from".
What's up?

int getenvelope(void *mc, const char *name, const char ***body)
{
    static const char *buf[2];

    if (buf[0] == NULL) { buf[0] = malloc(sizeof(char) * 256); buf[1] = NULL; }
    printf("Envelope body of '%s'? ", head);
    scanf("%s", buf[0]);
    body = buf;

    return SIEVE_OK;
}
*/

/* message action callbacks */

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
const char** errmsg // fill it in if you return failure
*/

static void action_log(sv_msg_ctx_t* mc, const char* action, const char* fmt, ...)
{
  const char* script = mc->sc->file;
  message_t msg = mc->msg;
  sv_action_log_t log = mc->sc->ic->action_log;
  va_list ap;

  va_start(ap, fmt);

  if(log)
    log(script, msg, action, fmt, ap);

  va_end(ap);
}

int
sv_keep (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t * m = (sv_msg_ctx_t *) mc;
  //sieve_keep_context_t * a = (sieve_keep_context_t *) ac;
  *errmsg = "keep";
  m->rc = 0;

  action_log (mc, "KEEP", "");

  return SIEVE_OK;
}

int
sv_fileinto (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sieve_fileinto_context_t *a = (sieve_fileinto_context_t *) ac;
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->rc = 0;

  action_log (mc, "FILEINTO", "save copy in %s", a->mailbox);

  if ((m->svflags & SV_FLAG_NO_ACTIONS) == 0)
    {
      *errmsg = "fileinto(saving)";
      m->rc = sv_mu_save_to (a->mailbox, m->msg, m->ticket, m->debug);
      if (!m->rc)
	{
	  *errmsg = "fileinto(saving)";
	  m->rc = sv_mu_mark_deleted (m->msg);
	}
    }

  return m->rc ? SIEVE_FAIL : SIEVE_OK;
}

int
sv_redirect (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  *errmsg = "redirect";
  m->rc = 0;

  action_log (mc, "REDIRECT", "");

  return SIEVE_FAIL;
}

int
sv_discard (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
//sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;

  *errmsg = "discard";
  m->rc = 0;

  action_log(mc, "DISCARD", "");

  if ((m->svflags & SV_FLAG_NO_ACTIONS) == 0)
    {
      m->rc = sv_mu_mark_deleted (m->msg);
    }

  return m->rc ? SIEVE_FAIL : SIEVE_OK;
}

int
sv_reject (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  *errmsg = "reject";
  m->rc = 0;

  action_log(mc, "REJECT", "");

  return SIEVE_FAIL;
}

#if 0
int sv_notify(void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
    sv_print_action("NOTIFY", ac, ic, sc, mc);

    return SIEVE_OK;
}
#endif

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

#if 0
sieve_vacation_t vacation = {
  0,				/* min response */
  0,				/* max response */
  &sv_autorespond,		/* autorespond() */
  &sv_send_response		/* send_response() */
};

char *markflags[] = { "\\flagged" };
sieve_imapflags_t mark = { markflags, 1 };
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
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) sc;

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
  if (rc == EOK)
    rc = sieve_register_size (i, &sv_getsize);
  if (rc == EOK)
    rc = sieve_register_header (i, &sv_getheader);
  if (rc == EOK)
    rc = sieve_register_redirect (i, &sv_redirect);
  if (rc == EOK)
    rc = sieve_register_keep (i, &sv_keep);

#if 0
  if (rc == EOK)
    rc = sieve_register_envelope (i, &sv_getenvelope);
#endif
  if (rc == EOK)
    rc = sieve_register_discard (i, &sv_discard);
#if 0
  if (rc == EOK)
    rc = sieve_register_reject (i, &sv_reject);
#endif
  /* The "fileinto" extension. */
  if (rc == EOK)
    rc = sieve_register_fileinto (i, &sv_fileinto);
#if 0
  if (rc == EOK)
    rc = sieve_register_vacation (i, &sv_vacation);
#endif
#if 0
  if (rc == EOK)
    rc = sieve_register_imapflags (i, &mark);
#endif
#if 0
  if (rc == EOK)
    rc = sieve_register_notify (i, &sv_notify);
#endif
  if (rc == EOK)
    rc = sieve_register_parse_error (i, &sv_parse_error);
  if (rc == EOK)
    rc = sieve_register_execute_error (i, &sv_execute_error);
  return rc;
}
