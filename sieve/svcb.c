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

  message_size (m->msg, &sz);

  *size = sz;

  sv_print (m->ic, SV_PRN_QRY, "getsize -> %d\n", *size);

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

      sv_print (m->ic, SV_PRN_QRY, "getheader, filling cache with %d fields\n", i);

      for (; i > 0; i--)
	{
	  m->rc = header_aget_field_name (h, i, &fn);
	  if (m->rc)
	    break;
	  m->rc = header_aget_field_value (h, i, &fv);
	  if (m->rc)
	    break;

	  sv_print (m->ic, SV_PRN_QRY, "getheader, cacheing %s=%s\n", fn, fv);

	  m->rc = sv_field_cache_add (&m->cache, fn, fv);

	  if (m->rc == 0)
	    {
	      fv = 0;		/* owned by the cache */
	    }
	  if (m->rc)
	    break;

	  /* the cache doesn't want it, and we don't need it */
	  free (fn);
	  fn = 0;
	}
      free (fn);
      free (fv);
    }
  if (!m->rc)
    {
      m->rc = sv_field_cache_get (&m->cache, name, body);
    }
  if (m->rc)
    {
      sv_print (m->ic, SV_PRN_QRY, "getheader %s, failed %s\n", name, strerror (m->rc));
    }
  else
    {
      const char **b = *body;
      int i = 1;
      sv_print (m->ic, SV_PRN_QRY, "getheader, %s=%s", name, b[0]);
      while (b[0] && b[i])
	{
	  sv_print (m->ic, SV_PRN_QRY, ", %s", b[i]);
	  i++;
	}
      sv_print (m->ic, SV_PRN_QRY, "\n");
    }
  return sv_mu_errno_to_rc (m->rc);
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

void
sv_print_action (const char* a, void *ac, void *ic, void *sc, void *mc)
{
//sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  sv_print (ic, SV_PRN_ACT, "action => %s\n", a);
}

int
sv_keep (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  //sv_msg_ctx_t * m = (sv_msg_ctx_t *) mc;
  //sieve_keep_context_t * a = (sieve_keep_context_t *) ac;

  sv_print_action ("KEEP", ac, ic, sc, mc);

  return SIEVE_OK;
}

int
sv_fileinto (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sieve_fileinto_context_t *a = (sieve_fileinto_context_t *) ac;
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;
  const char *what = "fileinto";
  int res = 0;

  sv_print_action ("FILEINTO", ac, ic, sc, mc);
  sv_print (i, SV_PRN_ACT, "  into <%s>\n", a->mailbox);

  if (!i->opt_no_actions)
    {
      res = sv_mu_save_to (a->mailbox, m->msg, i->ticket, &what);
    }

  if (res)
    {
      assert(what);

      *errmsg = strerror (res);
      sv_print (i, SV_PRN_ACT, "  %s failed with [%d] %s\n",
		what, res, *errmsg);
    }

  m->rc = res;

  return res ? SIEVE_FAIL : SIEVE_OK;
}

int
sv_redirect (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_print_action ("REDIRECT", ac, ic, sc, mc);

  return SIEVE_OK;
}

int
sv_discard (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;
  int res = 0;

  sv_print_action ("DISCARD", ac, ic, sc, mc);

  if (!i->opt_no_actions)
    {
      res = sv_mu_mark_deleted (m->msg);
    }
  if (res)
    *errmsg = strerror (res);

  return SIEVE_OK;
}

int
sv_reject (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_print_action ("REJECT", ac, ic, sc, mc);

  return SIEVE_OK;
}

/*
int sv_notify(void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
    sv_print_action("NOTIFY", ac, ic, sc, mc);

    return SIEVE_OK;
}
*/

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
  sv_interp_ctx_t* i = (sv_interp_ctx_t*) ic;

  sv_print (i, SV_PRN_ERR, "%s:%d: %s\n", i->opt_script, lineno, msg);

  return SIEVE_OK;
}

int
sv_execute_error (const char *msg, void *ic, void *sc, void *mc)
{
  sv_interp_ctx_t* i = (sv_interp_ctx_t*) ic;

  sv_print (i, SV_PRN_ERR, "sieve execute failed, %s\n", msg);

  return SIEVE_OK;
}

int
sv_summary (const char *msg, void *ic, void *sc, void *mc)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->summary = strdup (msg);

  return SIEVE_OK;
}

/* register all these callbacks */

int
sv_register_callbacks (sieve_interp_t * i)
{
  int res;

  res = sieve_register_size (i, &sv_getsize);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_size() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_header (i, &sv_getheader);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_header() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_redirect (i, &sv_redirect);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_redirect() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_keep (i, &sv_keep);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_keep() returns %d\n", res);
      exit (1);
    }
#if 0
  res = sieve_register_envelope (i, &sv_getenvelope);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_envelope() returns %d\n", res);
      exit (1);
    }
#endif
  res = sieve_register_discard (i, &sv_discard);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_discard() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_reject (i, &sv_reject);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_reject() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_fileinto (i, &sv_fileinto);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_fileinto() returns %d\n", res);
      exit (1);
    }
#if 0
  res = sieve_register_vacation (i, &sv_vacation);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_vacation() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_imapflags (i, &mark);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_imapflags() returns %d\n", res);
      exit (1);
    }
#endif

#if 0
  res = sieve_register_notify (i, &sv_notify);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_notify() returns %d\n", res);
      exit (1);
    }
#endif
  res = sieve_register_parse_error (i, &sv_parse_error);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_parse_error() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_execute_error (i, &sv_execute_error);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_execute_error() returns %d\n", res);
      exit (1);
    }
  res = sieve_register_summary (i, &sv_summary);
  if (res != SIEVE_OK)
    {
      printf ("sieve_register_summary() returns %d\n", res);
      exit (1);
    }
  return res;
}


