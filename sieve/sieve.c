/*
 * sieve interpreter
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <mailutils/mailbox.h>
#include <mailutils/address.h>
#include <mailutils/registrar.h>

#include "sieve_interface.h"
#include "message.h"

#include "svfield.h"

#ifndef EOK
# define EOK 0
#endif

/** utility wrappers around mailutils functionality **/

int
mu_copy_debug_level (const mailbox_t from, mailbox_t to)
{
  mu_debug_t d = 0;
  size_t level;
  int rc;

  if (!from || !to)
    return EINVAL;

  rc = mailbox_get_debug (from, &d);

  if (!rc)
    mu_debug_get_level (d, &level);

  if (!rc)
    rc = mailbox_get_debug (to, &d);

  if (!rc)
    mu_debug_set_level (d, level);

  return 0;
}

int
mu_save_to (const char *toname, message_t mesg, const char **errmsg)
{
  int res = 0;
  mailbox_t to = 0;
  mailbox_t from = 0;

  res = mailbox_create_default (&to, toname);

  if (res == ENOENT)
    *errmsg = "no handler for this type of mailbox";

  if (!res)
    {
      if (message_get_mailbox (mesg, &from) == 0)
	mu_copy_debug_level (from, to);
    }
  if (!res)
    {
      *errmsg = "mailbox_open";
      res = mailbox_open (to, MU_STREAM_WRITE | MU_STREAM_CREAT);
    }
  if (!res)
    {
      *errmsg = "mailbox_open";
      res = mailbox_append_message (to, mesg);

      if (!res)
	{
	  *errmsg = "mailbox_close";
	  res = mailbox_close (to);
	}
      else
	{
	  mailbox_close (to);
	}
    }
  mailbox_destroy (&to);

  return res;
}

int
mu_mark_deleted (message_t msg)
{
  attribute_t attr = 0;
  int res;

  res = message_get_attribute (msg, &attr);

  if (!res)
    attribute_set_deleted (attr);

  return res;
}

static int
sv_errno (int rc)
{
  switch (rc)
    {
    case ENOMEM:
      return SIEVE_NOMEM;
    case ENOENT:
      return SIEVE_FAIL;
    case EOK:
      return SIEVE_OK;
    }
  return SIEVE_INTERNAL_ERROR;
}

/** sieve context structures

The object relationship diagram is this, with the names in []
being the argument name when the context's are provided as
arguments to callback functions.

  sieve_execute_script()  --> sv_msg_ctx_t, "mc"

       |
       |
       V

  sieve_script_t  --->  sv_script_ctx_t, "sc"

       |
       |
       V

  sieve_interp_t  --->  sv_interp_ctx_t, "ic"


*/

typedef struct sv_interp_ctx_t
{
  /* cmd line options */
  int  opt_no_actions;
  int  opt_verbose;
  int  opt_no_run;
  int  opt_watch;
  char*opt_mbox;
  char*opt_script;

  int  print_mask;
  FILE*print_stream;

  /* mailutils debug handle, we need to destroy it */
  mu_debug_t debug;
} sv_interp_ctx_t;

typedef struct sv_script_ctx_t
{
    sv_interp_ctx_t* ic;
} sv_script_ctx_t;

typedef struct sv_msg_ctx_t
{
  int rc;			/* the mailutils return code */
  int cache_filled;
  sv_field_cache_t cache;
  message_t msg;
  mailbox_t mbox;
  char *summary;

  sv_interp_ctx_t* ic;
} sv_msg_ctx_t;

enum /* print level masks */
{
    SV_PRN_MU  = 0x1,
    SV_PRN_ACT = 0x2,
    SV_PRN_QRY = 0x4,
    SV_PRN_PRS = 0x6,
    SV_PRN_NOOP
};

void
sv_printv (sv_interp_ctx_t* ic, int level, const char *fmt, va_list ap)
{
  if(level & ic->print_mask)
    vfprintf(ic->print_stream, fmt, ap);
}
void sv_print (sv_interp_ctx_t* ic, int level, const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  sv_printv(ic, level, fmt, ap);
  va_end(ap);
}

/* we hook mailutils debug output into our diagnostics using this */
int
sv_mu_debug_print (mu_debug_t d, const char *fmt, va_list ap)
{
  sv_printv(mu_debug_get_owner(d), SV_PRN_MU, fmt, ap);

  return 0;
}

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
  return sv_errno (m->rc);
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
    *body = buf;

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

const char *
str_action (action_t a)
{
  switch (a)
    {
    case ACTION_NONE:
      return "NONE";
    case ACTION_REJECT:
      return "REJECT";
    case ACTION_FILEINTO:
      return "FILEINTO";
    case ACTION_KEEP:
      return "KEEP";
    case ACTION_REDIRECT:
      return "REDIRECT";
    case ACTION_DISCARD:
      return "DISCARD";
    case ACTION_VACATION:
      return "VACATION";
    case ACTION_SETFLAG:
      return "SETFLAG";
    case ACTION_ADDFLAG:
      return "ADDFLAG";
    case ACTION_REMOVEFLAG:
      return "REMOVEFLAG";
    case ACTION_MARK:
      return "MARK";
    case ACTION_UNMARK:
      return "UNMARK";
    case ACTION_NOTIFY:
      return "NOTIFY";
    case ACTION_DENOTIFY:
      return "DENOTIFY";
    }
  return "UNKNOWN";
}
const char *
str_sieve_error (int e)
{
  switch (e)
    {
    case SIEVE_FAIL:
      return "SIEVE_FAIL";
    case SIEVE_NOT_FINALIZED:
      return "SIEVE_NOT_FINALIZED";
    case SIEVE_PARSE_ERROR:
      return "SIEVE_PARSE_ERROR";
    case SIEVE_RUN_ERROR:
      return "SIEVE_RUN_ERROR";
    case SIEVE_INTERNAL_ERROR:
      return "SIEVE_INTERNAL_ERROR";
    case SIEVE_NOMEM:
      return "SIEVE_NOMEM";
    case SIEVE_DONE:
      return "SIEVE_DONE";
    }
  return "UNKNOWN";
}


void
sv_print_action (action_t a, void *ac, void *ic, void *sc, void *mc)
{
//sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  sv_print (ic, SV_PRN_ACT, "action => %s\n", str_action (a));
}

int
sv_keep (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  //sv_msg_ctx_t * m = (sv_msg_ctx_t *) mc;
  //sieve_keep_context_t * a = (sieve_keep_context_t *) ac;

  sv_print_action (ACTION_KEEP, ac, ic, sc, mc);

  return SIEVE_OK;
}

int
sv_fileinto (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sieve_fileinto_context_t *a = (sieve_fileinto_context_t *) ac;
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;
  int res = 0;

  sv_print_action (ACTION_FILEINTO, ac, ic, sc, mc);
  sv_print (i, SV_PRN_ACT, "  into <%s>\n", a->mailbox);

  if (!i->opt_no_actions)
    {
      res = mu_save_to (a->mailbox, m->msg, errmsg);

      if (!res)
	{
	  res = mu_mark_deleted (m->msg);
	}
    }
  if (res && !errmsg)
    *errmsg = strerror (res);

  if (res)
    sv_print (i, SV_PRN_ACT, "  fail with [%d] %s\n", res, *errmsg);

  return res ? SIEVE_FAIL : SIEVE_OK;
}

int
sv_redirect (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_print_action (ACTION_REDIRECT, ac, ic, sc, mc);

  return SIEVE_OK;
}

int
sv_discard (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;
  sv_interp_ctx_t *i = (sv_interp_ctx_t *) ic;
  int res = 0;

  sv_print_action (ACTION_DISCARD, ac, ic, sc, mc);

  if (!i->opt_no_actions)
    {
      res = mu_mark_deleted (m->msg);
    }
  if (res)
    *errmsg = strerror (res);

  return SIEVE_OK;
}

int
sv_reject (void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
  sv_print_action (ACTION_REJECT, ac, ic, sc, mc);

  return SIEVE_OK;
}

/*
int sv_notify(void *ac, void *ic, void *sc, void *mc, const char **errmsg)
{
    sv_print_action(ACTION_NOTIFY, ac, ic, sc, mc);

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

int
sv_parse_error (int lineno, const char *msg, void *ic, void *sc)
{
  sv_interp_ctx_t* i = (sv_interp_ctx_t*) ic;

  sv_print (i, SV_PRN_PRS, "%s:%d: %s\n", i->opt_script, lineno, msg);

  return SIEVE_OK;
}

int
sv_execute_error (const char *msg, void *ic, void *sc, void *mc)
{
  fprintf (stderr, "runtime error: %s\n", msg);

  return SIEVE_OK;
}

int
sv_summary (const char *msg, void *ic, void *sc, void *mc)
{
  sv_msg_ctx_t *m = (sv_msg_ctx_t *) mc;

  m->summary = strdup (msg);

  return SIEVE_OK;
}

int
sieve_register_mailutils (sieve_interp_t * i)
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

const char USAGE[] = "usage: sieve [-hnvcd] [-f mbox] script\n";

const char HELP[] =
  "	-h   print this helpful message and exit.\n"
  "	-n   no actions taken, implies -v.\n"
  "	-v   verbose, print actions to be taken.\n"
  "	-c   compile script but don't run it against an mbox.\n"
  "	-d   daemon mode, sieve mbox, then go into background and sieve.\n"
  "	     new message as they are delivered to mbox.\n"
  "	-f   the mbox to sieve, defaults to the users spool file.\n";

int
main (int argc, char *argv[])
{
  list_t bookie = 0;
  mailbox_t mbox = 0;

  sieve_interp_t *interp;
  sieve_script_t *script;

  sv_interp_ctx_t ic = { 0, };
  sv_script_ctx_t sc = { 0, };

  size_t count = 0;
  int i = 0;

  int res;
  FILE *f = 0;

  int opt;

  while ((opt = getopt (argc, argv, "hnvcdf:")) != -1)
    {
      switch (opt)
	{
	case 'h':
	  printf ("%s\n%s", USAGE, HELP);
	  return 0;
	case 'n':
	  ic.opt_no_actions = 1;
	case 'v':
	  ic.opt_verbose++;
	  break;
	case 'c':
	  ic.opt_no_run = 1;
	  break;
	case 'f':
	  ic.opt_mbox = optarg;
	  break;
	case 'd':
	  ic.opt_watch = 1;
	  break;
	default:
	  fprintf (stderr, "%s", USAGE);
	  return 1;
	}
    }
  if (!argv[optind])
    {
      fprintf (stderr, "%s", USAGE);
      return 1;
    }
  ic.opt_script = argv[optind];

  registrar_get_list (&bookie);
  list_append (bookie, path_record);
  list_append (bookie, file_record);
  list_append (bookie, mbox_record);
  list_append (bookie, pop_record);
  list_append (bookie, imap_record);

  if (!ic.opt_no_run && (res = mailbox_create_default (&mbox, ic.opt_mbox)) != 0)
    {
      fprintf (stderr, "mailbox create <%s> failed: %s\n",
	       ic.opt_mbox ? ic.opt_mbox : "default", strerror (res));
      return 1;
    }
  if (ic.opt_verbose > 2)
    {
      if ((res = mu_debug_create(&ic.debug, &ic)))
	{
	  fprintf (stderr, "mu_debug_create failed: %s\n", strerror(res));
	  return 1;
	}
      if ((res = mu_debug_set_level (ic.debug, MU_DEBUG_TRACE | MU_DEBUG_PROT)))
	{
	  fprintf (stderr, "mu_debug_set_level failed: %s\n", strerror(res));
	  return 1;
	}
      if ((res = mu_debug_set_print (ic.debug, sv_mu_debug_print, &ic)))
	{
	  fprintf (stderr, "mu_debug_set_print failed: %s\n", strerror(res));
	  return 1;
	}
      mailbox_set_debug (mbox, ic.debug);
    }
  if (ic.opt_no_actions)
    res = mailbox_open (mbox, MU_STREAM_READ);
  else
    res = mailbox_open (mbox, MU_STREAM_RDWR);

  if (res != 0)
    {
      fprintf (stderr, "mailbox open <%s> failed: %s\n",
	       ic.opt_mbox ? ic.opt_mbox : "default", strerror (res));
      exit (1);
    }
  res = sieve_interp_alloc (&interp, &ic);

  if (res != SIEVE_OK)
    {
      fprintf (stderr, "sieve_interp_alloc() returns %d\n", res);
      return 1;
    }
  res = sieve_register_mailutils (interp);

  f = fopen (ic.opt_script, "r");

  if (!f)
    {
      printf ("fopen %s failed: %s\n", argv[2], strerror (errno));
      return 1;
    }
  res = sieve_script_parse (interp, f, &sc, &script);
  if (res != SIEVE_OK)
    {
      printf ("script parse failed: %s\n", str_sieve_error (res));
      return 1;
    }
  fclose (f);

  if (ic.opt_no_run)
    return 0;

  mailbox_messages_count (mbox, &count);
  if (ic.opt_verbose)
    {
      fprintf (stderr, "mbox has %d messages...\n", count);
    }
  for (i = 1; i <= count; ++i)
    {
      sv_msg_ctx_t mc = { 0, };

      mc.mbox = mbox;

      if ((res = mailbox_get_message (mbox, i, &mc.msg)) != 0)
	{
	  fprintf (stderr, "mailbox_get_message(%d): %s\n", i,
		   strerror (res));
	  exit (1);
	}
      res = sieve_execute_script (script, &mc);

      if (ic.opt_verbose)
	fprintf (stderr, "%s\n", mc.summary);

      if (res != SIEVE_OK)
	{
	  printf ("sieve_execute_script(%d): sieve %d rc %s\n",
		  i, res, strerror (mc.rc));
	  exit (1);
	}
      sv_field_cache_release (&mc.cache);
      free (mc.summary);
    }
  res = sieve_script_free (&script);

  if (res != SIEVE_OK)
    {
      printf ("sieve_script_free() returns %d\n", res);
      exit (1);
    }
  res = sieve_interp_free (&interp);

  if (res != SIEVE_OK)
    {
      printf ("sieve_interp_free() returns %d\n", res);
      exit (1);
    }
  if (!ic.opt_no_actions)
    res = mailbox_expunge (mbox);

  if (res != 0)
    {
      printf ("mailbox_expunge failed: %s\n", strerror (res));
      exit (1);
    }
  if (res == 0)
    mailbox_close (mbox);

  mailbox_destroy (&mbox);

  return 0;
}

void
fatal (char *message, int rc)
{
  fprintf (stderr, "fatal error: %s\n", message);
  exit (rc);
}

