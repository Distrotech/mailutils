/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2003, 2007, 2009-2012 Free Software
   Foundation, Inc.

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

#include "pop3d.h"

/*
  The CAPA Command

  The POP3 CAPA command returns a list of capabilities supported by the
  POP3 server.  It is available in both the AUTHORIZATION and
  TRANSACTION states.

  Capabilities available in the AUTHORIZATION state MUST be announced
  in both states.  */

static int
print_capa (void *item, void *data)
{
  struct pop3d_capa *cp = item;
  struct pop3d_session *session = data;
  if (cp->type == capa_func)
    {
      cp->value.func (cp->name, session);
    }
  else 
    {
      pop3d_outf ("%s", cp->name);
      if (cp->value.string)
	pop3d_outf ("%s", cp->value.string);
      pop3d_outf ("\n");
    }
  return 0;
}

int
pop3d_capa (char *arg, struct pop3d_session *sess)
{
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  pop3d_outf ("+OK Capability list follows\n");
  mu_list_foreach (sess->capa, print_capa, sess);
  pop3d_outf (".\n");
  return OK;
}

static void
pop3d_append_capa_string (struct pop3d_session *sess, const char *name,
			  const char *value)
{
  struct pop3d_capa *cp;

  cp = mu_alloc (sizeof (*cp));
  cp->type = capa_string;
  cp->name = name;
  cp->value.string = value ? mu_strdup (value) : NULL;
  if (mu_list_append (sess->capa, cp))
    mu_alloc_die ();
}

static void
pop3d_append_capa_func (struct pop3d_session *sess, const char *name,
			void (*func) (const char *, struct pop3d_session *))
{
  struct pop3d_capa *cp;

  if (!func)
    return;
  cp = mu_alloc (sizeof (*cp));
  cp->type = capa_func;
  cp->name = name;
  cp->value.func = func;
  if (mu_list_append (sess->capa, cp))
    mu_alloc_die ();
}  

static void
capa_free (void *p)
{
  struct pop3d_capa *cp = p;
  if (cp->type == capa_string && cp->value.string)
    free (cp->value.string);
  free (cp);
}

static void
capa_implementation (const char *name, struct pop3d_session *session)
{
  if (state == TRANSACTION)	/* let's not advertise to just anyone */
    pop3d_outf ("%s %s\n", name, PACKAGE_STRING);
}

#ifdef WITH_TLS
static void
capa_stls (const char *name, struct pop3d_session *session)
{
  if ((session->tls == tls_ondemand || session->tls == tls_required)
      && tls_available && tls_done == 0)
    pop3d_outf ("%s\n", name);
}
#else
# define capa_stls NULL
#endif /* WITH_TLS */

static void
capa_user (const char *name, struct pop3d_session *session)
{
  if (state == INITIAL)
    pop3d_outf ("XTLSREQUIRED\n");
  else
    pop3d_outf ("USER\n");
}

void
pop3d_session_init (struct pop3d_session *session)
{
  if (mu_list_create (&session->capa))
    mu_alloc_die ();
  mu_list_set_destroy_item (session->capa, capa_free);

  /* Initialize the capability list */
  pop3d_append_capa_string (session, "TOP", NULL);
  pop3d_append_capa_string (session, "UIDL", NULL);
  pop3d_append_capa_string (session, "RESP-CODES", NULL);
  pop3d_append_capa_string (session, "PIPELINING", NULL);
  if (pop3d_xlines)
    pop3d_append_capa_string (session, "XLINES", NULL);

  pop3d_append_capa_func (session, "LOGIN-DELAY", login_delay_capa);
    
  /* This can be implemented by setting an header field on the message.  */
  pop3d_append_capa_string (session, "EXPIRE",
			    (expire == EXPIRE_NEVER) ?
			    "NEVER" : mu_umaxtostr (0, expire));

  pop3d_append_capa_func (session, NULL, capa_user);
  pop3d_append_capa_func (session, "STLS", capa_stls);
  pop3d_append_capa_func (session, "IMPLEMENTATION", capa_implementation);
}

void
pop3d_session_free (struct pop3d_session *session)
{
  mu_list_destroy (&session->capa);
}
