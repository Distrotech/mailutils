/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2006, 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

long message_tag;

struct mu_message
{
  mu_message_t msg;       /* Message itself */
  SCM mbox;               /* Mailbox it belongs to */
  int needs_destroy;      /* Set during mark phase if the message needs
			     explicit destroying */
};

/* SMOB functions: */

static SCM
mu_scm_message_mark (SCM message_smob)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (message_smob);
  if (mu_message_get_owner (mum->msg) == NULL)
    mum->needs_destroy = 1;
  return mum->mbox;
}

static scm_sizet
mu_scm_message_free (SCM message_smob)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (message_smob);
  if (mum->needs_destroy)
    mu_message_destroy (&mum->msg, NULL);
  free (mum);
  return sizeof (struct mu_message);
}

static char *
_get_envelope_sender (mu_envelope_t env)
{
  mu_address_t addr;
  char buffer[128];
  char *ptr;
  
  if (mu_envelope_sender (env, buffer, sizeof (buffer), NULL)
      || mu_address_create (&addr, buffer))
    return NULL;

  if (mu_address_aget_email (addr, 1, &ptr))
    {
      mu_address_destroy (&addr);
      return NULL;
    }

  mu_address_destroy (&addr);
  return ptr;
}

static int
mu_scm_message_print (SCM message_smob, SCM port, scm_print_state * pstate)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (message_smob);
  mu_envelope_t env = NULL;
  char buffer[128];
  const char *p;
  size_t m_size = 0, m_lines = 0;
  struct tm tm;
  mu_timezone tz;

  mu_message_get_envelope (mum->msg, &env);

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
      
      mu_envelope_date (env, buffer, sizeof (buffer), NULL);
      p = buffer;
      if (mu_parse_ctime_date_time (&p, &tm, &tz) == 0)
	strftime (buffer, sizeof (buffer), "%a %b %e %H:%M", &tm);
      else
	strcpy (buffer, "UNKNOWN");
      scm_puts ("\" \"", port);
      scm_puts (buffer, port);
      scm_puts ("\" ", port);
      
      mu_message_size (mum->msg, &m_size);
      mu_message_lines (mum->msg, &m_lines);
      
      snprintf (buffer, sizeof (buffer), "%3lu %-5lu",
		(unsigned long) m_lines, (unsigned long) m_size);
      scm_puts (buffer, port);
    }
  scm_puts (">", port);
  return 1;
}

/* Internal calls: */

SCM
mu_scm_message_create (SCM owner, mu_message_t msg)
{
  struct mu_message *mum;

  mum = scm_gc_malloc (sizeof (struct mu_message), "message");
  mum->msg = msg;
  mum->mbox = owner;
  mum->needs_destroy = 0;
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

const mu_message_t
mu_scm_message_get (SCM MESG)
{
  struct mu_message *mum = (struct mu_message *) SCM_CDR (MESG);
  return mum->msg;
}

int
mu_scm_is_message (SCM scm)
{
  return SCM_NIMP (scm) && (long) SCM_CAR (scm) == message_tag;
}

/* ************************************************************************* */
/* Guile primitives */

SCM_DEFINE (scm_mu_message_create, "mu-message-create", 0, 0, 0,
	    (),
	    "Creates an empty message.\n")
#define FUNC_NAME s_scm_mu_message_create
{
  mu_message_t msg;
  mu_message_create (&msg, NULL);
  return mu_scm_message_create (SCM_BOOL_F, msg);
}
#undef FUNC_NAME

/* FIXME: This changes envelope date */
SCM_DEFINE (scm_mu_message_copy, "mu-message-copy", 1, 0, 0,
	    (SCM MESG),
	    "Creates the copy of the message MESG.\n")
#define FUNC_NAME s_scm_mu_message_copy
{
  mu_message_t msg, newmsg;
  mu_stream_t in = NULL, out = NULL;
  char buffer[512];
  size_t off, n;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);

  status = mu_message_get_stream (msg, &in);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get input stream from message ~A",
		  scm_list_1 (MESG));
  
  status = mu_message_create (&newmsg, NULL);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot create message", SCM_BOOL_F);
  
  status = mu_message_get_stream (newmsg, &out);
  if (status)
    {
      mu_message_destroy (&newmsg, NULL);
      mu_scm_error (FUNC_NAME, status,
		    "Cannot get output stream", SCM_BOOL_F);
    }

  off = 0;
  while ((status = mu_stream_read (in, buffer, sizeof (buffer) - 1, off, &n))
	 == 0
	 && n != 0)
    {
      int wr;
      int rc;
      
      rc = mu_stream_write (out, buffer, n, off, &wr);
      if (rc)
	{
	  mu_message_destroy (&newmsg, NULL);
	  mu_scm_error (FUNC_NAME, rc, "Error writing to stream", SCM_BOOL_F);
	}
      
      off += n;
      if (wr != n)
	{
	  mu_message_destroy (&newmsg, NULL);
	  mu_scm_error (FUNC_NAME, rc, "Error writing to stream: Short write",
			SCM_BOOL_F);
	}
    }
  
  return mu_scm_message_create (SCM_BOOL_F, newmsg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_destroy, "mu-message-destroy", 1, 0, 0,
	    (SCM MESG),
	    "Destroys the message MESG.")
#define FUNC_NAME s_scm_mu_message_destroy
{
  struct mu_message *mum;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  mum = (struct mu_message *) SCM_CDR (MESG);
  mu_message_destroy (&mum->msg, mu_message_get_owner (mum->msg));
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_set_header, "mu-message-set-header", 3, 1, 0,
	    (SCM MESG, SCM HEADER, SCM VALUE, SCM REPLACE),
"Sets new VALUE to the header HEADER of the message MESG.\n"
"If HEADER is already present in the message its value\n"
"is replaced with the suplied one iff the optional REPLACE is\n"
"#t. Otherwise, a new header is created and appended.")
#define FUNC_NAME s_scm_mu_message_set_header
{
  mu_message_t msg;
  mu_header_t hdr;
  int replace = 0;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (scm_is_string (HEADER), HEADER, SCM_ARG2, FUNC_NAME);

  if (SCM_IMP (VALUE) && SCM_BOOLP (VALUE))
    return SCM_UNSPECIFIED;
  
  SCM_ASSERT (scm_is_string (VALUE), VALUE, SCM_ARG2, FUNC_NAME);
  if (!SCM_UNBNDP (REPLACE))
    {
      replace = REPLACE == SCM_BOOL_T;
    }
  
  status = mu_message_get_header (msg, &hdr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message headers", SCM_BOOL_F);

  status = mu_header_set_value (hdr, scm_i_string_chars (HEADER),
				scm_i_string_chars (VALUE),
				replace);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot set header \"~A: ~A\" in message ~A",
		  scm_list_3 (HEADER, VALUE, MESG));
  
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_size, "mu-message-get-size", 1, 0, 0,
	    (SCM MESG),
	    "Returns the size of the message MESG\n.")
#define FUNC_NAME s_scm_mu_message_get_size
{
  mu_message_t msg;
  size_t size;
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  mu_message_size (msg, &size);
  return mu_scm_makenum (size);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_lines, "mu-message-get-lines", 1, 0, 0,
	    (SCM MESG),
	    "Returns number of lines in the given message.\n")
#define FUNC_NAME s_scm_mu_message_get_lines
{
  mu_message_t msg;
  size_t lines;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  status = mu_message_lines (msg, &lines);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get number of lines in message ~A",
		  scm_list_1 (MESG));

  return mu_scm_makenum (lines);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_sender, "mu-message-get-sender", 1, 0, 0,
	    (SCM MESG),
	    "Returns email address of the sender of the message MESG.\n")
#define FUNC_NAME s_scm_mu_message_get_sender
{
  mu_message_t msg;
  mu_envelope_t env = NULL;
  int status;
  SCM ret;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  status = mu_message_get_envelope (msg, &env);
  if (status == 0)
    {
      char *p = _get_envelope_sender (env);
      ret = scm_makfrom0str (p);
      free (p);
    }
  else
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get envelope of message ~A",
		  scm_list_1 (MESG));
  return ret;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_header, "mu-message-get-header", 2, 0, 0,
	    (SCM MESG, SCM HEADER),
	    "Returns value of the header HEADER from the message MESG.\n")
#define FUNC_NAME s_scm_mu_message_get_header
{
  mu_message_t msg;
  mu_header_t hdr;
  char *value = NULL;
  const char *header_string;
  SCM ret;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (scm_is_string (HEADER), HEADER, SCM_ARG2, FUNC_NAME);
  header_string = scm_i_string_chars (HEADER);
  status = mu_message_get_header (msg, &hdr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message headers", SCM_BOOL_F);

  status = mu_header_aget_value (hdr, header_string, &value);
  switch (status)
    {
    case 0:
      ret = scm_makfrom0str (value);
      free (value);
      break;
      
    case MU_ERR_NOENT:
      ret = SCM_BOOL_F;
      break;

    default:
      mu_scm_error (FUNC_NAME, status,
		    "Cannot get header ~A from message ~A",
		    scm_list_2 (HEADER, MESG));
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
      if (scm_is_string (car)
	  && strcasecmp (scm_i_string_chars (car), name) == 0)
	return 1;
    }
  return 0;
}

SCM_DEFINE (scm_mu_message_get_header_fields, "mu-message-get-header-fields", 1, 1, 0,
	    (SCM MESG, SCM HEADERS),
"Returns the list of headers in the message MESG. Optional argument\n" 
"HEADERS gives a list of header names to restrict return value to.\n")
#define FUNC_NAME s_scm_mu_message_get_header_fields
{
  size_t i, nfields = 0;
  mu_message_t msg;
  mu_header_t hdr = NULL;
  SCM scm_first = SCM_EOL, scm_last;
  SCM headers = SCM_EOL;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  if (!SCM_UNBNDP (HEADERS))
    {
      SCM_ASSERT (SCM_NIMP (HEADERS) && SCM_CONSP (HEADERS),
		  HEADERS, SCM_ARG2, FUNC_NAME);
      headers = HEADERS;
    }

  status = mu_message_get_header (msg, &hdr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message headers", SCM_BOOL_F);
  status = mu_header_get_field_count (hdr, &nfields);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get header field count", SCM_BOOL_F);
  
  for (i = 1; i <= nfields; i++)
    {
      SCM new_cell, scm_name, scm_value;
      char *name, *value;
      
      status = mu_header_aget_field_name (hdr, i, &name);
      if (status)
	mu_scm_error (FUNC_NAME, status,
		      "Cannot get header field ~A, message ~A",
		      scm_list_2 (scm_from_size_t (i), MESG));
      
      if (headers != SCM_EOL && string_sloppy_member (headers, name) == 0)
	continue;
      status = mu_header_aget_field_value (hdr, i, &value);
      if (status)
	mu_scm_error (FUNC_NAME, status,
		      "Cannot get header value ~A, message ~A",
		      scm_list_2 (scm_from_size_t (i), MESG));

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
  
SCM_DEFINE (scm_mu_message_set_header_fields, "mu-message-set-header-fields", 2, 1, 0,
	    (SCM MESG, SCM LIST, SCM REPLACE),
"Set the headers in the message MESG from LIST\n"
"LIST is a list of conses (cons HEADER VALUE).  The function sets\n"
"these headers in the message MESG.\n"
"Optional parameter REPLACE specifies whether the new header\n"
"values should replace the headers already present in the\n"
"message.")
#define FUNC_NAME s_scm_mu_message_set_header_fields
{
  mu_message_t msg;
  mu_header_t hdr;
  SCM list;
  int replace = 0;
  int status;
  
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

  status = mu_message_get_header (msg, &hdr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message headers", SCM_BOOL_F);

  for (list = LIST; list != SCM_EOL; list = SCM_CDR (list))
    {
      SCM cell = SCM_CAR (list);
      SCM car, cdr;
      
      SCM_ASSERT (SCM_NIMP (cell) && SCM_CONSP (cell),
		  cell, SCM_ARGn, FUNC_NAME);
      car = SCM_CAR (cell);
      cdr = SCM_CDR (cell);
      SCM_ASSERT (scm_is_string (car), car, SCM_ARGn, FUNC_NAME);
      SCM_ASSERT (scm_is_string (cdr), cdr, SCM_ARGn, FUNC_NAME);
      status = mu_header_set_value (hdr,
				    scm_i_string_chars (car),
				    scm_i_string_chars (cdr), replace);
      if (status)
	mu_scm_error (FUNC_NAME, status,
		      "Cannot set header value: message ~A, header ~A, value ~A",
		      scm_list_3 (MESG, car, cdr));

    }
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_delete, "mu-message-delete", 1, 1, 0,
	    (SCM MESG, SCM FLAG),
"Mark the message MESG as deleted. Optional argument FLAG allows to toggle\n"
"deletion mark. The message is deleted if it is @code{#t} and undeleted if\n"
"it is @code{#f}")
#define FUNC_NAME s_scm_mu_message_delete
{
  mu_message_t msg;
  mu_attribute_t attr;
  int delete = 1;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  if (!SCM_UNBNDP (FLAG))
    {
      SCM_ASSERT (SCM_IMP (FLAG) && SCM_BOOLP (FLAG),
		  FLAG, SCM_ARG2, FUNC_NAME);
      delete = FLAG == SCM_BOOL_T;
    }
  status = mu_message_get_attribute (msg, &attr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message attribute", SCM_BOOL_F);

  if (delete)
    status = mu_attribute_set_deleted (attr);
  else
    status = mu_attribute_unset_deleted (attr);
  
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Error setting message attribute", SCM_BOOL_F);

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_flag, "mu-message-get-flag", 2, 0, 0,
	    (SCM MESG, SCM FLAG),
"Return value of the attribute FLAG of the message MESG.")
#define FUNC_NAME s_scm_mu_message_get_flag
{
  mu_message_t msg;
  mu_attribute_t attr;
  int ret = 0;
  int status;
    
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (scm_is_integer (FLAG), FLAG, SCM_ARG2, FUNC_NAME);

  status = mu_message_get_attribute (msg, &attr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message attribute", SCM_BOOL_F);
  
  switch (scm_to_int32 (FLAG))
    {
    case MU_ATTRIBUTE_ANSWERED:
      ret = mu_attribute_is_answered (attr);
      break;
      
    case MU_ATTRIBUTE_FLAGGED:
      ret = mu_attribute_is_flagged (attr);
      break;
      
    case MU_ATTRIBUTE_DELETED:
      ret = mu_attribute_is_deleted (attr);
      break;
      
    case MU_ATTRIBUTE_DRAFT:
      ret = mu_attribute_is_draft (attr);
      break;
      
    case MU_ATTRIBUTE_SEEN:
      ret = mu_attribute_is_seen (attr);
      break;
      
    case MU_ATTRIBUTE_READ:
      ret = mu_attribute_is_read (attr);
      break;
      
    case MU_ATTRIBUTE_MODIFIED:
      ret = mu_attribute_is_modified (attr);
      break;
      
    case MU_ATTRIBUTE_RECENT:
      ret = mu_attribute_is_recent (attr);
      break;
      
    default:
      mu_attribute_get_flags (attr, &ret);
      ret &= scm_to_int32 (FLAG);
    }
  return ret ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_set_flag, "mu-message-set-flag", 2, 1, 0,
	    (SCM MESG, SCM FLAG, SCM VALUE),
"Set the attribute FLAG of the message MESG. If optional VALUE is #f, the\n"
"attribute is unset.")
#define FUNC_NAME s_scm_mu_message_set_flag
{
  mu_message_t msg;
  mu_attribute_t attr;
  int value = 1;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (scm_is_integer (FLAG), FLAG, SCM_ARG2, FUNC_NAME);

  if (!SCM_UNBNDP (VALUE))
    {
      SCM_ASSERT (SCM_IMP (VALUE) && SCM_BOOLP (VALUE),
		  VALUE, SCM_ARG3, FUNC_NAME);
      value = VALUE == SCM_BOOL_T;
    }
  
  status = mu_message_get_attribute (msg, &attr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message attribute", SCM_BOOL_F);

  status = 0;
  switch (scm_to_int32 (FLAG))
    {
    case MU_ATTRIBUTE_ANSWERED:
      if (value)
	status = mu_attribute_set_answered (attr);
      else
	status = mu_attribute_unset_answered (attr);
      break;
      
    case MU_ATTRIBUTE_FLAGGED:
      if (value)
	status = mu_attribute_set_flagged (attr);
      else
	status = mu_attribute_unset_flagged (attr);
      break;
      
    case MU_ATTRIBUTE_DELETED:
      if (value)
	status = mu_attribute_set_deleted (attr);
      else
	status = mu_attribute_unset_deleted (attr);
      break;
      
    case MU_ATTRIBUTE_DRAFT:
      if (value)
	status = mu_attribute_set_draft (attr);
      else
	status = mu_attribute_unset_draft (attr);
      break;
      
    case MU_ATTRIBUTE_SEEN:
      if (value)
	status = mu_attribute_set_seen (attr);
      else
	status = mu_attribute_unset_seen (attr);
      break;
      
    case MU_ATTRIBUTE_READ:
      if (value)
	status = mu_attribute_set_read (attr);
      else
	status = mu_attribute_unset_read (attr);
      break;
      
    case MU_ATTRIBUTE_MODIFIED:
      if (value)
	status = mu_attribute_set_modified (attr);
      else
	status = mu_attribute_clear_modified (attr);
      break;
      
    case MU_ATTRIBUTE_RECENT:
      if (value)
	status = mu_attribute_set_recent (attr);
      else
	status = mu_attribute_unset_recent (attr);
      break;
      
    default:
      if (value)
	status = mu_attribute_set_flags (attr, scm_to_int32 (FLAG));
    }
  
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Error setting message attribute", SCM_BOOL_F);
  
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_user_flag, "mu-message-get-user-flag", 2, 0, 0,
	    (SCM MESG, SCM FLAG),
"Return the value of the user attribute FLAG from the message MESG.")
#define FUNC_NAME s_scm_mu_message_get_user_flag
{
  mu_message_t msg;
  mu_attribute_t attr;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (scm_is_integer (FLAG), FLAG, SCM_ARG2, FUNC_NAME);
  status = mu_message_get_attribute (msg, &attr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message attribute", SCM_BOOL_F);
  return mu_attribute_is_userflag (attr, scm_to_int32 (FLAG)) ?
                                SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME
  

SCM_DEFINE (scm_mu_message_set_user_flag, "mu-message-set-user-flag", 2, 1, 0,
	    (SCM MESG, SCM FLAG, SCM VALUE),
"Set the given user attribute FLAG in the message MESG. If optional argumen\n"
"VALUE is @samp{#f}, the attribute is unset.")
#define FUNC_NAME s_scm_mu_message_set_user_flag
{
  mu_message_t msg;
  mu_attribute_t attr;
  int set = 1;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  SCM_ASSERT (scm_is_integer (FLAG), FLAG, SCM_ARG2, FUNC_NAME);

  if (!SCM_UNBNDP (VALUE))
    {
      SCM_ASSERT (SCM_IMP (VALUE) && SCM_BOOLP (VALUE),
		  VALUE, SCM_ARG3, FUNC_NAME);
      set = VALUE == SCM_BOOL_T;
    }
  
  status = mu_message_get_attribute (msg, &attr);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get message attribute", SCM_BOOL_F);
  
  if (set)
    mu_attribute_set_userflag (attr, scm_to_int32 (FLAG));
  else
    mu_attribute_unset_userflag (attr, scm_to_int32 (FLAG));
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_port, "mu-message-get-port", 2, 1, 0,
	    (SCM MESG, SCM MODE, SCM FULL),
"Returns a port associated with the given MESG. MODE is a string\n"
"defining operation mode of the stream. It may contain any of the\n"
"two characters: @samp{r} for reading, @samp{w} for writing.\n"
"If optional argument FULL is specified, it should be a boolean value.\n"
"If it is @samp{#t} then the returned port will allow access to any\n"
"part of the message (including headers). If it is @code{#f} then the port\n"
"accesses only the message body (the default).\n")
#define FUNC_NAME s_scm_mu_message_get_port
{
  mu_message_t msg;
  mu_stream_t stream = NULL;
  int status;
    
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_string (MODE), MODE, SCM_ARG2, FUNC_NAME);

  msg = mu_scm_message_get (MESG);

  if (!SCM_UNBNDP (FULL))
    {
      SCM_ASSERT (SCM_IMP (FULL) && SCM_BOOLP (FULL),
		  FULL, SCM_ARG3, FUNC_NAME);
      if (FULL == SCM_BOOL_T)
	{
	  status = mu_message_get_stream (msg, &stream);
	  if (status)
	    mu_scm_error (FUNC_NAME, status, "Cannot get message stream",
			  SCM_BOOL_F);
	}
    }

  if (!stream)
    {
      mu_body_t body = NULL;

      status = mu_message_get_body (msg, &body);
      if (status)
	mu_scm_error (FUNC_NAME, status, "Cannot get message body",
		      SCM_BOOL_F);
      status = mu_body_get_stream (body, &stream);
      if (status)
	mu_scm_error (FUNC_NAME, status, "Cannot get message body stream",
		      SCM_BOOL_F);
    }
  
  return mu_port_make_from_stream (MESG, stream,
			   scm_mode_bits ((char*)scm_i_string_chars (MODE))); 
}
#undef FUNC_NAME
  

SCM_DEFINE (scm_mu_message_get_body, "mu-message-get-body", 1, 0, 0,
	    (SCM MESG),
	    "Returns the message body for the message MESG.")
#define FUNC_NAME s_scm_mu_message_get_body
{
  mu_message_t msg;
  mu_body_t body = NULL;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  status = mu_message_get_body (msg, &body);
  if (status)
    mu_scm_error (FUNC_NAME, status, "Cannot get message body", SCM_BOOL_F);
  return mu_scm_body_create (MESG, body);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_multipart_p, "mu-message-multipart?", 1, 0, 0,
	    (SCM MESG),
	    "Returns @code{#t} if MESG is a multipart @acronym{MIME} message.")
#define FUNC_NAME s_scm_mu_message_multipart_p
{
  mu_message_t msg;
  int ismime = 0;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  mu_message_is_multipart (msg, &ismime);
  return ismime ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_num_parts, "mu-message-get-num-parts", 1, 0, 0,
	    (SCM MESG),
"Returns number of parts in a multipart @acronym{MIME} message. Returns\n"
"@code{#f} if the argument is not a multipart message.")
#define FUNC_NAME s_scm_mu_message_get_num_parts
{
  mu_message_t msg;
  int ismime = 0;
  size_t nparts = 0;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  mu_message_is_multipart (msg, &ismime);
  if (!ismime)
    return SCM_BOOL_F;

  status = mu_message_get_num_parts (msg, &nparts);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get number of parts in the message ~A",
		  scm_list_1 (MESG));
  return mu_scm_makenum (nparts);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_part, "mu-message-get-part", 2, 0, 0,
	    (SCM MESG, SCM PART),
"Returns part #PART from a multipart @acronym{MIME} message MESG.")
#define FUNC_NAME s_scm_mu_message_get_part
{
  mu_message_t msg, submsg;
  int ismime = 0;
  int status;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  SCM_ASSERT (scm_is_integer (PART), PART, SCM_ARG2, FUNC_NAME);

  msg = mu_scm_message_get (MESG);
  mu_message_is_multipart (msg, &ismime);
  if (!ismime)
    return SCM_BOOL_F;

  status = mu_message_get_part (msg, scm_to_int32 (PART), &submsg);
  if (status)
    mu_scm_error (FUNC_NAME, status,
		  "Cannot get number of part ~A from the message ~A",
		  scm_list_2 (PART, MESG));

  return mu_scm_message_create (MESG, submsg);
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_send, "mu-message-send", 1, 3, 0,
	    (SCM MESG, SCM MAILER, SCM FROM, SCM TO),
"Sends the message MESG. Optional MAILER overrides default mailer settings\n"
"in mu-mailer. Optional FROM and TO give sender and recever addresses.\n")
#define FUNC_NAME s_scm_mu_message_send
{
  const char *mailer_name;
  mu_address_t from = NULL;
  mu_address_t to = NULL;
  mu_mailer_t mailer = NULL;
  mu_message_t msg;
  int status;

  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  
  if (!SCM_UNBNDP (MAILER) && MAILER != SCM_BOOL_F)
    {
      SCM_ASSERT (scm_is_string (MAILER), MAILER, SCM_ARG2, FUNC_NAME);
      mailer_name = scm_i_string_chars (MAILER);
    }
  else
    {
      SCM val = MU_SCM_SYMBOL_VALUE("mu-mailer");
      mailer_name = scm_i_string_chars(val);
    }
  
  if (!SCM_UNBNDP (FROM) && FROM != SCM_BOOL_F)
    {
      SCM_ASSERT (scm_is_string (FROM)
		  && mu_address_create (&from, scm_i_string_chars (FROM)) == 0,
		  FROM, SCM_ARG3, FUNC_NAME);
    }
  
  if (!SCM_UNBNDP (TO) && TO != SCM_BOOL_F)
    {
      SCM_ASSERT (scm_is_string (TO)
		  && mu_address_create (&to, scm_i_string_chars (TO)) == 0,
		  TO, SCM_ARG4, FUNC_NAME);
    }

  status = mu_mailer_create (&mailer, mailer_name);
  if (status)
    mu_scm_error (FUNC_NAME, status, "Cannot get create mailer", SCM_BOOL_F);

  if (scm_to_int32 (MU_SCM_SYMBOL_VALUE ("mu-debug")))
    {
      mu_debug_t debug = NULL;
      mu_mailer_get_debug (mailer, &debug);
      mu_debug_set_level (debug, MU_DEBUG_TRACE|MU_DEBUG_PROT);
    }

  status = mu_mailer_open (mailer, MU_STREAM_RDWR);
  if (status == 0)
    {
      status = mu_mailer_send_message (mailer, msg, from, to);
      if (status)
	mu_scm_error (FUNC_NAME, status, "Cannot send message", SCM_BOOL_F);

      mu_mailer_close (mailer);
    }
  else
    mu_scm_error (FUNC_NAME, status, "Cannot open mailer", SCM_BOOL_F);
  mu_mailer_destroy (&mailer);
  
  return status == 0 ? SCM_BOOL_T : SCM_BOOL_F;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_message_get_uid, "mu-message-get-uid", 1, 0, 0,
	    (SCM MESG),
	    "Returns uid of the message MESG\n")
#define FUNC_NAME s_scm_mu_message_get_uid
{
  mu_message_t msg;
  int status;
  size_t uid;
  
  SCM_ASSERT (mu_scm_is_message (MESG), MESG, SCM_ARG1, FUNC_NAME);
  msg = mu_scm_message_get (MESG);
  status = mu_message_get_uid (msg, &uid);
  if (status)
    mu_scm_error (FUNC_NAME, status, "Cannot get message uid", SCM_BOOL_F);
  return scm_from_size_t (uid);
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
