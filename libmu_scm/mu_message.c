/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mu_scm.h"

long message_tag;

struct mu_message
{
  message_t msg;       /* Message itself */
  SCM mbox;            /* Mailbox it belongs to */
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
  if (message_get_owner (mum->msg) == NULL)
    message_destroy (&mum->msg, NULL);
  free (mum);
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

  if (message_smob == SCM_BOOL_F)
    {
      /* several mu_message.* functions may return #f */
      scm_puts ("#f", port);
    }
  else
    {
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
      
      snprintf (buffer, sizeof (buffer), "%3lu %-5lu",
		(unsigned long) m_lines, (unsigned long) m_size);
      scm_puts (buffer, port);
    }
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

void
mu_scm_message_add_owner (SCM MESG, SCM owner)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (MESG);
  SCM cell;

  if (SCM_IMP (mum->mbox) && SCM_BOOLP (mum->mbox))
    {
      mum->mbox = owner;
      return;
    }
  
  SCM_NEWCELL (cell);
  SCM_SETCAR (cell, owner);
  if (SCM_NIMP (mum->mbox) && SCM_CONSP (mum->mbox))
    SCM_SETCDR (cell, mum->mbox);
  else
    {
      SCM scm;
      SCM_NEWCELL (scm);
      SCM_SETCAR (scm, mum->mbox);
      SCM_SETCDR (scm, SCM_EOL);
      SCM_SETCDR (cell, scm);
    }
  mum->mbox = cell;
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

/* FIXME: This changes envelope date */
SCM_DEFINE (mu_message_copy, "mu-message-copy", 1, 0, 0,
	    (SCM MESG),
	    "Creates the copy of the given message.\n")
#define FUNC_NAME s_mu_message_copy
{
  message_t msg, newmsg;
  stream_t in = NULL, out = NULL;
  char buffer[512];
  size_t off, n;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);

  if (message_get_stream (msg, &in))
    return SCM_BOOL_F;
  
  if (message_create (&newmsg, NULL))
    return SCM_BOOL_F;
  
  if (message_get_stream (newmsg, &out))
    {
      message_destroy (&newmsg, NULL);
      return SCM_BOOL_F;
    }

  off = 0;
  while (stream_read (in, buffer, sizeof (buffer) - 1, off, &n) == 0
	 && n != 0)
    {
      int wr;
      
      stream_write (out, buffer, n, off, &wr);
      off += n;
      if (wr != n)
	{
	  message_destroy (&newmsg, NULL);
	  return SCM_BOOL_F;
	}
    }
  
  return mu_scm_message_create (SCM_BOOL_F, newmsg);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_destroy, "mu-message-destroy", 1, 0, 0,
	    (SCM MESG),
	    "Destroys the message.")
#define FUNC_NAME s_mu_message_destroy
{
  struct mu_message *mum;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_message *) SCM_CDR (MESG);
  message_destroy (&mum->msg, message_get_owner (mum->msg));
  return SCM_UNSPECIFIED;
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

  if (SCM_IMP (VALUE) && SCM_BOOLP (VALUE))
    return SCM_UNSPECIFIED;
  
  SCM_ASSERT (SCM_NIMP (VALUE) && SCM_STRINGP (VALUE),
	      VALUE, SCM_ARG2, FUNC_NAME);
  if (!SCM_UNBNDP (REPLACE))
    {
      replace = REPLACE == SCM_BOOL_T;
    }
  
  message_get_header (msg, &hdr);
  header_set_value (hdr, SCM_STRING_CHARS (HEADER), SCM_STRING_CHARS (VALUE),
		    replace);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_size, "mu-message-get-size", 1, 0, 0,
	    (SCM MESG),
	    "Returns the size of the given message.")
#define FUNC_NAME s_mu_message_get_size
{
  message_t msg;
  size_t size;
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  message_size (msg, &size);
  return scm_makenum (size);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_lines, "mu-message-get-lines", 1, 0, 0,
	    (SCM MESG),
	    "Returns number of lines in the given message.")
#define FUNC_NAME s_mu_message_get_lines
{
  message_t msg;
  size_t lines;
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  message_lines (msg, &lines);
  return scm_makenum (lines);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_sender, "mu-message-get-sender", 1, 0, 0,
	    (SCM MESG),
	    "Returns the sender email address for the message MESG.")
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
  header_string = SCM_STRING_CHARS (HEADER);
  message_get_header (msg, &hdr);
  if (header_aget_value (hdr, header_string, &value) == 0)
    {
      ret = scm_makfrom0str (value);
      free (value);
    }
  return ret;
}
#undef FUNC_NAME

static int
string_sloppy_member (SCM lst, char *name)
{
  for(; SCM_CONSP (lst); lst = SCM_CDR(lst))
    {
      SCM car = SCM_CAR (lst);
      if ((SCM_NIMP (car) && SCM_STRINGP (car))
	  && strcasecmp (SCM_STRING_CHARS (car), name) == 0)
	return 1;
    }
  return 0;
}

SCM_DEFINE (mu_message_get_header_fields, "mu-message-get-header-fields", 1, 1, 0,
	    (SCM MESG, SCM HEADERS),
	    "Returns the list of headers in the MESG. If optional HEADERS is\n"
	    "specified it should be a list of header names to restrict return\n"
	    "value to.\n")
#define FUNC_NAME s_mu_message_get_header_fields
{
  size_t i, nfields = 0;
  message_t msg;
  header_t hdr = NULL;
  SCM scm_first = SCM_EOL, scm_last;
  SCM headers = SCM_EOL;
    
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  if (!SCM_UNBNDP (HEADERS))
    {
      SCM_ASSERT (SCM_NIMP (HEADERS) && SCM_CONSP (HEADERS),
		  HEADERS, SCM_ARG2, FUNC_NAME);
      headers = HEADERS;
    }

  message_get_header (msg, &hdr);
  header_get_field_count (hdr, &nfields);
  for (i = 1; i <= nfields; i++)
    {
      SCM new_cell, scm_name, scm_value;
      char *name, *value;
      
      header_aget_field_name (hdr, i, &name);
      if (headers != SCM_EOL && string_sloppy_member (headers, name) == 0)
	continue;
      header_aget_field_value (hdr, i, &value);

      scm_name = scm_makfrom0str (name);
      scm_value = scm_makfrom0str (value);

      SCM_NEWCELL(new_cell);
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

  message_get_header (msg, &hdr);
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
      header_set_value (hdr, SCM_STRING_CHARS (car), SCM_STRING_CHARS (cdr), replace);
    }
  return SCM_UNDEFINED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_delete, "mu-message-delete", 1, 1, 0,
	    (SCM MESG, SCM FLAG),
	    "Mark given message as deleted. Optional FLAG allows to toggle deleted mark\n"
	    "The message is deleted if it is #t and undeleted if it is #f")
#define FUNC_NAME s_mu_message_delete
{
  message_t msg;
  attribute_t attr;
  int delete = 1;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  if (!SCM_UNBNDP (FLAG))
    {
      SCM_ASSERT (SCM_IMP (FLAG) && SCM_BOOLP (FLAG),
		  FLAG, SCM_ARG2, FUNC_NAME);
      delete = FLAG == SCM_BOOL_T;
    }
  message_get_attribute (msg, &attr);
  if (delete)
    attribute_set_deleted (attr);
  else
    attribute_unset_deleted (attr);
  return SCM_UNDEFINED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_flag, "mu-message-get-flag", 2, 0, 0,
	    (SCM MESG, SCM FLAG),
	    "Return value of the attribute FLAG.")
#define FUNC_NAME s_mu_message_get_flag
{
  message_t msg;
  attribute_t attr;
  int ret = 0;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (SCM_IMP (FLAG) && SCM_INUMP (FLAG), FLAG, SCM_ARG2, FUNC_NAME);

  message_get_attribute (msg, &attr);
  switch (SCM_INUM (FLAG))
    {
    case MU_ATTRIBUTE_ANSWERED:
      ret = attribute_is_answered (attr);
      break;
    case MU_ATTRIBUTE_FLAGGED:
      ret = attribute_is_flagged (attr);
      break;
    case MU_ATTRIBUTE_DELETED:
      ret = attribute_is_deleted (attr);
      break;
    case MU_ATTRIBUTE_DRAFT:
      ret = attribute_is_draft (attr);
      break;
    case MU_ATTRIBUTE_SEEN:
      ret = attribute_is_seen (attr);
      break;
    case MU_ATTRIBUTE_READ:
      ret = attribute_is_read (attr);
      break;
    case MU_ATTRIBUTE_MODIFIED:
      ret = attribute_is_modified (attr);
      break;
    case MU_ATTRIBUTE_RECENT:
      ret = attribute_is_recent (attr);
      break;
    default:
      attribute_get_flags (attr, &ret);
      ret &= SCM_INUM (FLAG);
    }
  return ret ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_set_flag, "mu-message-set-flag", 2, 1, 0,
	    (SCM MESG, SCM FLAG, SCM VALUE),
	    "Set the given attribute of the message. If optional VALUE is #f, the\n"
	    "attribute is unset.")
#define FUNC_NAME s_mu_message_set_flag
{
  message_t msg;
  attribute_t attr;
  int value = 1;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (SCM_IMP (FLAG) && SCM_INUMP (FLAG), FLAG, SCM_ARG2, FUNC_NAME);

  if (!SCM_UNBNDP (VALUE))
    {
      SCM_ASSERT (SCM_IMP (VALUE) && SCM_BOOLP (VALUE),
		  VALUE, SCM_ARG3, FUNC_NAME);
      value = VALUE == SCM_BOOL_T;
    }
  
  message_get_attribute (msg, &attr);
  switch (SCM_INUM (FLAG))
    {
    case MU_ATTRIBUTE_ANSWERED:
      if (value)
	attribute_set_answered (attr);
      else
	attribute_unset_answered (attr);
      break;
    case MU_ATTRIBUTE_FLAGGED:
      if (value)
	attribute_set_flagged (attr);
      else
	attribute_unset_flagged (attr);
      break;
    case MU_ATTRIBUTE_DELETED:
      if (value)
	attribute_set_deleted (attr);
      else
	attribute_unset_deleted (attr);
      break;
    case MU_ATTRIBUTE_DRAFT:
      if (value)
	attribute_set_draft (attr);
      else
	attribute_unset_draft (attr);
      break;
    case MU_ATTRIBUTE_SEEN:
      if (value)
	attribute_set_seen (attr);
      else
	attribute_unset_seen (attr);
      break;
    case MU_ATTRIBUTE_READ:
      if (value)
	attribute_set_read (attr);
      else
	attribute_unset_read (attr);
      break;
    case MU_ATTRIBUTE_MODIFIED:
      if (value)
	attribute_set_modified (attr);
      else
	attribute_clear_modified (attr);
      break;
    case MU_ATTRIBUTE_RECENT:
      if (value)
	attribute_set_recent (attr);
      else
	attribute_unset_recent (attr);
      break;
    default:
      if (value)
	attribute_set_flags (attr, SCM_INUM (FLAG));
    }
  return SCM_UNDEFINED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_user_flag, "mu-message-get-user-flag", 2, 0, 0,
	    (SCM MESG, SCM FLAG),
	    "Returns value of the user attribute FLAG.")
#define FUNC_NAME s_mu_message_get_user_flag
{
  message_t msg;
  attribute_t attr;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (SCM_IMP (FLAG) && SCM_INUMP (FLAG), FLAG, SCM_ARG2, FUNC_NAME);
  message_get_attribute (msg, &attr);
  return attribute_is_userflag (attr, SCM_INUM (FLAG)) ?
                                SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME
  

SCM_DEFINE (mu_message_set_user_flag, "mu-message-set-user-flag", 2, 1, 0,
	    (SCM MESG, SCM FLAG, SCM VALUE),
	    "Set the given user attribute of the message. If optional VALUE is\n"
	    "#f, the attribute is unset.")
#define FUNC_NAME s_mu_message_set_user_flag
{
  message_t msg;
  attribute_t attr;
  int set = 1;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (SCM_IMP (FLAG) && SCM_INUMP (FLAG), FLAG, SCM_ARG2, FUNC_NAME);

  if (!SCM_UNBNDP (VALUE))
    {
      SCM_ASSERT (SCM_IMP (VALUE) && SCM_BOOLP (VALUE),
		  VALUE, SCM_ARG3, FUNC_NAME);
      set = VALUE == SCM_BOOL_T;
    }
  
  message_get_attribute (msg, &attr);
  if (set)
    attribute_set_userflag (attr, SCM_INUM (FLAG));
  else
    attribute_unset_userflag (attr, SCM_INUM (FLAG));
  return SCM_UNDEFINED;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_port, "mu-message-get-port", 2, 1, 0,
	    (SCM MESG, SCM MODE, SCM FULL),
	    "Returns a port associated with the given MESG. MODE is a string\n"
	    "defining operation mode of the stream. It may contain any of the\n"
	    "two characters: \"r\" for reading, \"w\" for writing.\n"
	    "If optional FULL argument specified, it should be a boolean value.\n"
	    "If it is #t then the returned port will allow access to any\n"
	    "part of the message (including headers). If it is #f then the port\n"
	    "accesses only the message body (the default).\n")
#define FUNC_NAME s_mu_message_get_port
{
  message_t msg;
  stream_t stream = NULL;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (SCM_NIMP (MODE) && SCM_STRINGP (MODE),
	      MODE, SCM_ARG2, FUNC_NAME);

  msg = mu_scm_message_get (MESG);

  if (!SCM_UNBNDP (FULL))
    {
      SCM_ASSERT (SCM_IMP (FULL) && SCM_BOOLP (FULL),
		  FULL, SCM_ARG3, FUNC_NAME);
      if (FULL == SCM_BOOL_T && message_get_stream (msg, &stream))
	return SCM_BOOL_F;
    }

  if (!stream)
    {
      body_t body = NULL;

      if (message_get_body (msg, &body)
	  || body_get_stream (body, &stream))
	return SCM_BOOL_F;
    }
  
  return mu_port_make_from_stream (MESG, stream,
				   scm_mode_bits (SCM_STRING_CHARS (MODE)));    
}
#undef FUNC_NAME
  

SCM_DEFINE (mu_message_get_body, "mu-message-get-body", 1, 0, 0,
	    (SCM MESG),
	    "Returns the message body for the message MESG.")
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

SCM_DEFINE (mu_message_multipart_p, "mu-message-multipart?", 1, 0, 0,
	    (SCM MESG),
	    "Returns #t if MESG is a multipart MIME message.")
#define FUNC_NAME s_mu_message_multipart_p
{
  message_t msg;
  int ismime = 0;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  message_is_multipart (msg, &ismime);
  return ismime ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_num_parts, "mu-message-get-num-parts", 1, 0, 0,
	    (SCM MESG),
	    "Returns number of parts in a multipart MIME message. Returns\n"
            "#f if the argument is not a multipart message.")
#define FUNC_NAME s_mu_message_get_num_parts
{
  message_t msg;
  int ismime = 0;
  size_t nparts = 0;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  message_is_multipart (msg, &ismime);
  if (!ismime)
    return SCM_BOOL_F;

  message_get_num_parts (msg, &nparts);
  return scm_makenum (nparts);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_get_part, "mu-message-get-part", 2, 0, 0,
	    (SCM MESG, SCM PART),
	    "Returns part number PART from a multipart MIME message.")
#define FUNC_NAME s_mu_message_get_part
{
  message_t msg, submsg;
  int ismime = 0;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (SCM_IMP (PART) && SCM_INUMP (PART), PART, SCM_ARG2, FUNC_NAME);

  msg = mu_scm_message_get (MESG);
  message_is_multipart (msg, &ismime);
  if (!ismime)
    return SCM_BOOL_F;

  if (message_get_part (msg, SCM_INUM (PART), &submsg))
    return SCM_BOOL_F;
  return mu_scm_message_create (MESG, submsg);
}
#undef FUNC_NAME

SCM_DEFINE (mu_message_send, "mu-message-send", 1, 3, 0,
	    (SCM MESG, SCM MAILER, SCM FROM, SCM TO),
	    "Sends the message MESG. Optional MAILER overrides default\n"
	    "mailer settings in mu-mailer.\n"
	    "Optional FROM and TO are sender and recever addresses\n")
#define FUNC_NAME s_mu_message_send
{
  char *mailer_name;
  address_t from = NULL;
  address_t to = NULL;
  mailer_t mailer = NULL;
  message_t msg;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  
  if (!SCM_UNBNDP (MAILER) && MAILER != SCM_BOOL_F)
    {
      SCM_ASSERT (SCM_NIMP (MAILER) && SCM_STRINGP (MAILER),
		  MAILER, SCM_ARG2, FUNC_NAME);
      mailer_name = SCM_STRING_CHARS (MAILER);
    }
  else
    mailer_name = SCM_STRING_CHARS(_mu_scm_mailer);

  if (!SCM_UNBNDP (FROM) && FROM != SCM_BOOL_F)
    {
      SCM_ASSERT (SCM_NIMP (FROM) && SCM_STRINGP (FROM)
		  && address_create (&from, SCM_STRING_CHARS (FROM)) == 0,
		  FROM, SCM_ARG3, FUNC_NAME);
    }
  
  if (!SCM_UNBNDP (TO) && TO != SCM_BOOL_F)
    {
      SCM_ASSERT (SCM_NIMP (TO) && SCM_STRINGP (TO)
		  && address_create (&to, SCM_STRING_CHARS (TO)) == 0,
		  TO, SCM_ARG4, FUNC_NAME);
    }

  if (mailer_create (&mailer, mailer_name))
    {
      return SCM_BOOL_F;
    }

  if (SCM_INUM(_mu_scm_debug))
    {
      mu_debug_t debug = NULL;
      mailer_get_debug (mailer, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
    }

  status = mailer_open (mailer, MU_STREAM_RDWR);
  if (status == 0)
    {
      status = mailer_send_message (mailer, msg, from, to);
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
