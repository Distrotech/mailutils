/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mu_scm.h"

long message_tag;

struct mu_message
{
  message_t msg;       /* Message itself */
  SCM mbox;            /* Mailbox it belong to */
};

/* SMOB functions: */

static SCM
mu_scm_message_mark (SCM message_smob)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (message_smob);
  return mum->mbox;
}

static scm_sizet
mu_scm_message_free (SCM message_smob)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (message_smob);
  free (mum);
  /* NOTE: Currently there is no way for this function to return the
     amount of memory *actually freed* by mailbox_destroy */
  return sizeof (struct mu_message);
}

static char *
_get_envelope_sender (envelope_t env)
{
  address_t addr;
  char buffer[128];

  if (envelope_sender (env, buffer, sizeof (buffer), NULL)
      || address_create (&addr, buffer))
    return NULL;

  if (address_get_email (addr, 1, buffer, sizeof (buffer), NULL))
    {
      address_destroy (&addr);
      return NULL;
    }

  address_destroy (&addr);
  return strdup (buffer);
}

static int
mu_scm_message_print (SCM message_smob, SCM port, scm_print_state * pstate)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (message_smob);
  envelope_t env = NULL;
  char buffer[128];
  const char *p;
  size_t m_size = 0, m_lines = 0;
  struct tm tm;
  mu_timezone tz;

  message_get_envelope (mum->msg, &env);

  scm_puts ("#<message ", port);

  p = _get_envelope_sender (env);
  scm_puts ("\"", port);
  if (p)
    {
      scm_puts (p, port);
      free ((void *) p);
    }
  else
    scm_puts ("UNKNOWN", port);

  envelope_date (env, buffer, sizeof (buffer), NULL);
  p = buffer;
  if (mu_parse_ctime_date_time (&p, &tm, &tz) == 0)
    strftime (buffer, sizeof (buffer), "%a %b %e %H:%M", &tm);
  else
    strcpy (buffer, "UNKNOWN");
  scm_puts ("\" \"", port);
  scm_puts (buffer, port);
  scm_puts ("\" ", port);

  message_size (mum->msg, &m_size);
  message_lines (mum->msg, &m_lines);

  snprintf (buffer, sizeof (buffer), "%3ld %-5ld", m_lines, m_size);
  scm_puts (buffer, port);

  scm_puts (">", port);
  return 1;
}

/* Internal calls: */

SCM
mu_scm_message_create (SCM owner, message_t msg)
{
  struct mu_message *mum;

  mum = scm_must_malloc (sizeof (struct mu_message), "message");
  mum->msg = msg;
  mum->mbox = owner;
  SCM_RETURN_NEWSMOB (message_tag, mum);
}

const message_t
mu_scm_message_get (SCM MESG)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (MESG);
  return mum->msg;
}

int
mu_scm_is_message (SCM scm)
{
  return SCM_NIMP (scm) && SCM_CAR (scm) == message_tag;
}

/* ************************************************************************* */
/* Guile primitives */

SCM_DEFINE (mu_message_create, "mu-message-create", 0, 0, 0,
	    (),
	    "Creates an empty message.")
#define FUNC_NAME s_mu_message_create
{
  message_t msg;
  message_create (&msg, NULL);
  return mu_scm_message_create (SCM_BOOL_F, msg);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_set_header, "mu-message-set-header", 3, 1, 0,
	    (SCM MESG, SCM HEADER, SCM VALUE, SCM REPLACE),
	    "Sets new VALUE to the header HEADER of the message MESG.\n"
	    "If the HEADER is already present in the message its value\n"
	    "is replaced with the suplied one iff the optional REPLACE is\n"
	    "#t. Otherwise new header is created and appended.")
#define FUNC_NAME s_mu_message_set_header
{
  message_t msg;
  header_t hdr;
  int replace = 0;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (SCM_NIMP (HEADER) && SCM_STRINGP (HEADER),
	      HEADER, SCM_ARG2, FUNC_NAME);
  SCM_ASSERT (SCM_NIMP (VALUE) && SCM_STRINGP (VALUE),
	      VALUE, SCM_ARG2, FUNC_NAME);
  if (!SCM_UNBNDP (REPLACE))
    {
      SCM_ASSERT (SCM_IMP (REPLACE) && SCM_INUMP (REPLACE),
		  REPLACE, SCM_ARG3, FUNC_NAME);
      replace = REPLACE == SCM_BOOL_T;
    }
  
  message_get_header (msg, &hdr);
  header_set_value (hdr, SCM_CHARS (HEADER), strdup (SCM_CHARS (VALUE)),
		    replace);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


SCM_DEFINE (mu_message_get_sender, "mu-message-get-sender", 1, 0, 0,
	    (SCM MESG),
	    "Returns the sender email address for the message MESG")
#define FUNC_NAME s_mu_message_get_sender
{
  message_t msg;
  envelope_t env = NULL;
  SCM ret = SCM_BOOL_F;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  if (message_get_envelope (msg, &env) == 0)
    {
      char *p = _get_envelope_sender (env);
      ret = scm_makfrom0str (p);
      free (p);
    }
  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_header, "mu-message-get-header", 2, 0, 0,
	    (SCM MESG, SCM HEADER),
	    "Returns the header value of the HEADER in the MESG.")
#define FUNC_NAME s_mu_message_get_header
{
  message_t msg;
  header_t hdr;
  char *value = NULL;
  char *header_string;
  SCM ret = SCM_BOOL_F;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (SCM_NIMP (HEADER) && SCM_STRINGP (HEADER),
	      HEADER, SCM_ARG2, FUNC_NAME);
  header_string = SCM_CHARS (HEADER);
  message_get_header (msg, &hdr);
  if (header_aget_value (hdr, header_string, &value) == 0)
    {
      ret = scm_makfrom0str (value);
      free (value);
    }
  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_header_fields, "mu-message-get-header-fields", 1, 0, 0,
	    (SCM MESG),
	    "Returns the list of headers in the MESG.")
#define FUNC_NAME s_mu_message_get_header_fields
{
  size_t i, nfields = 0;
  message_t msg;
  header_t hdr = NULL;
  SCM scm_first = SCM_EOL, scm_last;
    
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);

  message_get_header (msg, &hdr);
  header_get_field_count (hdr, &nfields);
  for (i = 1; i <= nfields; i++)
    {
      SCM new_cell, scm_name, scm_value;
      char *name, *value;
      
      SCM_NEWCELL(new_cell);
      header_aget_field_name (hdr, i, &name);
      header_aget_field_value (hdr, i, &value);

      scm_name = scm_makfrom0str (name);
      scm_value = scm_makfrom0str (value);

      SCM_SETCAR(new_cell, scm_cons(scm_name, scm_value));
      
      if (scm_first == SCM_EOL)
	{
	  scm_first = new_cell;
	  scm_last = scm_first;
	}
      else
	{
	  SCM_SETCDR(scm_last, new_cell);
	  scm_last = new_cell;
	}
    }
  if (scm_first != SCM_EOL)
    SCM_SETCDR(scm_last, SCM_EOL);
  return scm_first;
}
#undef FUNC_NAME
  
SCM_DEFINE (mu_message_set_header_fields, "mu-message-set-header-fields", 2, 1, 0,
	    (SCM MESG, SCM LIST, SCM REPLACE),
	    "Set the headers in the message MESG from LIST\n"
	    "LIST is a list of (cons HEADER VALUE)\n"
	    "Optional parameter REPLACE spceifies whether the new header\n"
	    "values should replace the headers already present in the\n"
	    "message.")
#define FUNC_NAME s_mu_message_set_header_fields
{
  message_t msg;
  header_t hdr;
  SCM list;
  int replace = 0;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (((SCM_IMP (LIST) && SCM_EOL == LIST) ||
	       (SCM_NIMP (LIST) && SCM_CONSP (LIST))),
	      LIST, SCM_ARG2, FUNC_NAME);
  if (!SCM_UNBNDP (REPLACE))
    {
      SCM_ASSERT (SCM_IMP (REPLACE) && SCM_BOOLP (REPLACE),
		  REPLACE, SCM_ARG3, FUNC_NAME);
      replace = REPLACE == SCM_BOOL_T;
    }
  else

  for (list = LIST; list != SCM_EOL; list = SCM_CDR (list))
    {
      SCM cell = SCM_CAR (list);
      SCM car, cdr;
      
      SCM_ASSERT(SCM_NIMP(cell) && SCM_CONSP(cell),
		 cell, SCM_ARGn, FUNC_NAME);
      car = SCM_CAR (cell);
      cdr = SCM_CDR (cell);
      SCM_ASSERT (SCM_NIMP (car) && SCM_STRINGP (car),
		  car, SCM_ARGn, FUNC_NAME);
      SCM_ASSERT (SCM_NIMP (cdr) && SCM_STRINGP (cdr),
		  cdr, SCM_ARGn, FUNC_NAME);
      header_set_value (hdr, SCM_CHARS (car), SCM_CHARS (cdr), replace);
    }
  return SCM_UNDEFINED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_body, "mu-message-get-body", 1, 0, 0,
	    (SCM MESG),
	    "Returns the message body for the message MESG")
#define FUNC_NAME s_mu_message_get_body
{
  message_t msg;
  body_t body = NULL;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  if (message_get_body (msg, &body))
    return SCM_BOOL_F;
  return mu_scm_body_create (MESG, body);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_send, "mu-message-send", 1, 1, 0,
	    (SCM MESG, SCM MAILER),
	    "Sends the message MESG. Optional MAILER overrides default\n"
	    "mailer settings in mu-mailer.\n")
#define FUNC_NAME s_mu_message_send
{
  char *mailer_name;
  mailer_t mailer;
  message_t msg;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  
  if (!SCM_UNBNDP (MAILER))
    {
      SCM_ASSERT (SCM_NIMP (MAILER) && SCM_STRINGP (MAILER),
		  MAILER, SCM_ARG2, FUNC_NAME);
      mailer_name = SCM_CHARS (MAILER);
    }
  else
    mailer_name = SCM_CHARS(_mu_scm_mailer);

  if (mailer_create (&mailer, mailer_name))
    {
      return SCM_BOOL_F;
    }

  if (SCM_INUM(_mu_scm_debug))
    {
      debug_t debug = NULL;
      mailer_get_debug (mailer, &debug);
      debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
    }

  status = mailer_open (mailer, MU_STREAM_RDWR);
  if (status == 0)
    {
      status = mailer_send_message (mailer, msg);
      mailer_close (mailer);
    }
  mailer_destroy (&mailer);
  
  return status == 0 ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

/* Initialize the module */

void
mu_scm_message_init ()
{
  message_tag = scm_make_smob_type ("message", sizeof (struct mu_message));
  scm_set_smob_mark (message_tag, mu_scm_message_mark);
  scm_set_smob_free (message_tag, mu_scm_message_free);
  scm_set_smob_print (message_tag, mu_scm_message_print);

#include "mu_message.x"

}
