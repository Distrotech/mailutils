
#include "sv.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

const char *
sv_strerror (int e)
{
  switch (e)
    {
    case SV_EPARSE:
      return "parse failure";
    }
  return mu_errstring (e);
}

int
sv_interp_alloc (sv_interp_t * ip,
		 sv_parse_error_t pe,
		 sv_execute_error_t ee, sv_action_log_t al)
{
  int rc = 0;

  sv_interp_t i = calloc (1, sizeof (struct sv_interp_ctx_t));

  if (!i)
    return ENOMEM;

  i->parse_error = pe;
  i->execute_error = ee;
  i->action_log = al;

  if (sieve_interp_alloc (&i->interp, i) != SIEVE_OK)
    {
      free (i);
      return ENOMEM;
    }

  if ((rc = sv_register_callbacks (i->interp)))
    {
      sieve_interp_free (&i->interp);
      free (i);
      return rc;
    }

  *ip = i;

  return 0;
}

void
sv_interp_free (sv_interp_t * ip)
{
  sv_interp_t i = *ip;

  if (i)
    {
      if (i->interp)
	sieve_interp_free (&i->interp);

      free (i);
    }
  *ip = 0;
}

int
sv_script_parse (sv_script_t * sp, sv_interp_t i, const char *script)
{
  FILE *f = 0;
  sv_script_t s = 0;
  int rc = 0;

  if (!i || !script)
    return EINVAL;

  if ((s = calloc (1, sizeof (struct sv_script_ctx_t))) == 0)
    return ENOMEM;

  s->ic = i;

  if ((s->file = strdup (script)) == 0)
    {
      free (s);
      return ENOMEM;
    }

  f = fopen (script, "r");

  if (!f)
    {
      free (s->file);
      free (s);
      return errno;
    }

  rc = sieve_script_parse (i->interp, f, s, &s->script);

  fclose (f);

  switch (rc)
    {
    case SIEVE_NOT_FINALIZED:
      rc = EINVAL;
      break;
    case SIEVE_PARSE_ERROR:
      rc = ENOEXEC;
      break;
    case SIEVE_NOMEM:
      rc = ENOMEM;
      break;
    }

  if (rc)
    {
      free (s->file);
      free (s);
    }
  else
    *sp = s;

  return rc;
}

void
sv_script_free (sv_script_t * sp)
{
  sv_script_t s = *sp;

  if (s)
    {
      if (s->script)
	sieve_script_free (&s->script);

      free (s->file);
      free (s);
    }
  *sp = 0;
}

int
sv_script_execute (sv_script_t script, message_t msg, ticket_t ticket,
		   mu_debug_t debug, mailer_t mailer, int svflags)
{
  sv_msg_ctx_t mc = { 0, };
  int rc;
  int isdeleted;
  attribute_t attr = 0;

  rc = message_get_attribute (msg, &attr);

  if(rc)
    return rc;

  isdeleted = attribute_is_deleted(attr);

  mc.sc = script;
  mc.msg = msg;
  mc.ticket = ticket;
  mc.debug = debug;
  mc.mailer = mailer;
  mc.svflags = svflags;

  message_get_uid (msg, &mc.uid);

  rc = sieve_execute_script (script->script, &mc);

  sv_field_cache_release (&mc.cache);

  if(rc && mc.rc)
    rc = mc.rc;

  /* Reset the deleted flag if the script failed. */
  if(rc)
    sv_mu_mark_deleted (msg, isdeleted);

  return rc;
}

