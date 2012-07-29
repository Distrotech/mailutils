/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2005-2012 Free Software Foundation, Inc.

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

#include "imap4d.h"
#include <ctype.h>
#include <mailutils/argcv.h>

/*  Taken from RFC2060
    fetch           ::= "FETCH" SPACE set SPACE ("ALL" / "FULL" /
    "FAST" / fetch_att / "(" 1#fetch_att ")")

    fetch_att       ::= "ENVELOPE" / "FLAGS" / "INTERNALDATE" /
    "RFC822" [".HEADER" / ".SIZE" / ".TEXT"] /
    "BODY" ["STRUCTURE"] / "UID" /
    "BODY" [".PEEK"] section
    ["<" number "." nz_number ">"]
*/

struct fetch_runtime_closure
{
  int eltno;           /* Serial number of the last output FETCH element */
  size_t msgno;        /* Sequence number of the message being processed */
  mu_message_t msg;    /* The message itself */
  mu_list_t msglist;   /* A list of referenced messages.  See KLUDGE below. */
  char *err_text;      /* On return: error description if failed. */

  mu_list_t fnlist;
};

struct fetch_function_closure;

typedef int (*fetch_function_t) (struct fetch_function_closure *,
				 struct fetch_runtime_closure *);

struct fetch_function_closure
{
  fetch_function_t fun;            /* Handler function */
  const char *name;                /* Response tag */
  const char *section_tag;          
  size_t *section_part;            /* Section-part */
  size_t nset;                     /* Number of elements in section_part */
  int peek;
  int not;                         /* Negate header set */  
  mu_list_t headers;               /* Headers */
  size_t start;                    /* Substring start */ 
  size_t size;                     /* Substring length */
};

struct fetch_parse_closure
{
  int isuid;
  mu_list_t fnlist;
  mu_msgset_t msgset;
};


static int
fetch_send_address (const char *addr)
{
  mu_address_t address;
  size_t i, count = 0;

  /* Short circuit.  */
  if (addr == NULL || *addr == '\0')
    {
      io_sendf ("NIL");
      return RESP_OK;
    }

  mu_address_create (&address, addr);
  mu_address_get_count (address, &count);

  /* We failed: can't parse.  */
  if (count == 0)
    {
      io_sendf ("NIL");
      return RESP_OK;
    }

  io_sendf ("(");
  for (i = 1; i <= count; i++)
    {
      const char *str;
      int is_group = 0;

      io_sendf ("(");

      mu_address_sget_personal (address, i, &str);
      io_send_qstring (str);
      io_sendf (" ");

      mu_address_sget_route (address, i, &str);
      io_send_qstring (str);

      io_sendf (" ");

      mu_address_is_group (address, i, &is_group);
      str = NULL;
      if (is_group)
	mu_address_sget_personal (address, i, &str);
      else
	mu_address_sget_local_part (address, i, &str);

      io_send_qstring (str);

      io_sendf (" ");

      mu_address_sget_domain (address, i, &str);
      io_send_qstring (str);

      io_sendf (")");
    }
  io_sendf (")");
  return RESP_OK;
}

static void
fetch_send_header_value (mu_header_t header, const char *name,
			 const char *defval, int space)
{
  char *buffer;
  
  if (space)
    io_sendf (" ");
  if (mu_header_aget_value (header, name, &buffer) == 0)
    {
      io_send_qstring (buffer);
      free (buffer);
    }
  else if (defval)
    io_send_qstring (defval);
  else
    io_sendf ("NIL");
}

static void
fetch_send_header_address (mu_header_t header, const char *name,
			   const char *defval, int space)
{
  char *buffer;
  
  if (space)
    io_sendf (" ");
  if (mu_header_aget_value (header, name, &buffer) == 0)
    {
      fetch_send_address (buffer);
      free (buffer);
    }
  else
    fetch_send_address (defval);
}

static void
imap4d_ws_alloc_die (struct mu_wordsplit *wsp)
{
  imap4d_bye (ERR_NO_MEM);
}

#define IMAP4D_WS_FLAGS \
  (MU_WRDSF_DEFFLAGS | MU_WRDSF_DELIM | \
   MU_WRDSF_ENOMEMABRT | MU_WRDSF_ALLOC_DIE)

/* Send parameter list for the bodystructure.  */
static void
send_parameter_list (const char *buffer)
{
  struct mu_wordsplit ws;
  
  if (!buffer)
    {
      io_sendf ("NIL");
      return;
    }

  ws.ws_delim = " \t\r\n;=";
  ws.ws_alloc_die = imap4d_ws_alloc_die;
  if (mu_wordsplit (buffer, &ws, IMAP4D_WS_FLAGS))
    {
      mu_error (_("%s failed: %s"), "mu_wordsplit",
		mu_wordsplit_strerror (&ws));
      return; /* FIXME: a better error handling, maybe? */
    }
  
  if (ws.ws_wordc == 0)
    io_sendf ("NIL");
  else
    {
      char *p;
      
      io_sendf ("(");
        
      p = ws.ws_wordv[0];
      io_send_qstring (p);

      if (ws.ws_wordc > 1)
	{
	  int i, space = 0;
	  char *lvalue = NULL;

	  io_sendf ("(");
	  for (i = 1; i < ws.ws_wordc; i++)
	    {
	      if (lvalue)
		{
		  if (space)
		    io_sendf (" ");
		  io_send_qstring (lvalue);
		  lvalue = NULL;
		  space = 1;
		}
	      
	      switch (ws.ws_wordv[i][0])
		{
		case ';':
		  continue;
		  
		case '=':
		  if (++i < ws.ws_wordc)
		    {
		      io_sendf (" ");
		      io_send_qstring (ws.ws_wordv[i]);
		    }
		  break;
		  
		default:
		  lvalue = ws.ws_wordv[i];
		}
	    }
	  if (lvalue)
	    {
	      if (space)
		io_sendf (" ");
	      io_send_qstring (lvalue);
	    }
	  io_sendf (")");
	}
      else
	io_sendf (" NIL");
      io_sendf (")");
    }
  mu_wordsplit_free (&ws);
}

static void
fetch_send_header_list (mu_header_t header, const char *name,
			const char *defval, int space)
{
  char *buffer;
  
  if (space)
    io_sendf (" ");
  if (mu_header_aget_value (header, name, &buffer) == 0)
    {
      send_parameter_list (buffer);
      free (buffer);
    }
  else if (defval)
    send_parameter_list (defval);
  else
    io_sendf ("NIL");
}

/* ENVELOPE:
   The envelope structure of the message.  This is computed by the server by
   parsing the [RFC-822] header into the component parts, defaulting various
   fields as necessary.  The fields are presented in the order:
   Date, Subject, From, Sender, Reply-To, To, Cc, Bcc, In-Reply-To, Message-ID.
   Any field of an envelope or address structure that is not applicable is
   presented as NIL.  Note that the server MUST default the reply-to and sender
   fields from the from field.  The date, subject, in-reply-to, and message-id
   fields are strings.  The from, sender, reply-to, to, cc, and bcc fields
   are parenthesized lists of address structures.  */
static int
fetch_envelope0 (mu_message_t msg)
{
  char *from = NULL;
  mu_header_t header = NULL;

  mu_message_get_header (msg, &header);

  fetch_send_header_value (header, "Date", NULL, 0);
  fetch_send_header_value (header, "Subject", NULL, 1);

  /* From:  */
  mu_header_aget_value (header, "From", &from);
  io_sendf (" ");
  fetch_send_address (from);

  fetch_send_header_address (header, "Sender", from, 1);
  fetch_send_header_address (header, "Reply-to", from, 1);
  fetch_send_header_address (header, "To", NULL, 1);
  fetch_send_header_address (header, "Cc", NULL, 1);
  fetch_send_header_address (header, "Bcc", NULL, 1);
  fetch_send_header_value (header, "In-Reply-To", NULL, 1);
  fetch_send_header_value (header, "Message-ID", NULL, 1);

  free (from);
  return RESP_OK;
}

static int fetch_bodystructure0 (mu_message_t message, int extension);

/* The basic fields of a non-multipart body part are in the following order:
   body type:
   A string giving the content media type name as defined in [MIME-IMB].

   body subtype:
   A string giving the content subtype name as defined in [MIME-IMB].

   body parameter parenthesized list:
   A parenthesized list of attribute/value pairs [e.g. ("foo" "bar" "baz"
   "rag") where "bar" is the value of "foo" and "rag" is the value of "baz"]
   as defined in [MIME-IMB].

   body id:
   A string giving the content id as defined in [MIME-IMB].

   body description:
   A string giving the content description as defined in [MIME-IMB].

   body encoding:
   A string giving the content transfer encoding as defined in [MIME-IMB].

   body size:
   A number giving the size of the body in octets. Note that this size is the
   size in its transfer encoding and not the resulting size after any decoding.

   A body type of type TEXT contains, immediately after the basic fields, the
   size of the body in text lines.

   A body type of type MESSAGE and subtype RFC822 contains, immediately after
   the basic fields, the envelope structure, body structure, and size in text
   lines of the encapsulated message.

   The extension data of a non-multipart body part are in the following order:
   body MD5:
   A string giving the body MD5 value as defined in [MD5].

   body disposition:
   A parenthesized list with the same content and function as the body
   disposition for a multipart body part.

   body language:\
   A string or parenthesized list giving the body language value as defined
   in [LANGUAGE-TAGS].
 */
static int
bodystructure (mu_message_t msg, int extension)
{
  mu_header_t header = NULL;
  char *buffer = NULL;
  size_t blines = 0;
  int message_rfc822 = 0;
  int text_plain = 0;

  mu_message_get_header (msg, &header);

  if (mu_header_aget_value (header, MU_HEADER_CONTENT_TYPE, &buffer) == 0)
    {
      struct mu_wordsplit ws;
      char *p;
      size_t len;
      
      ws.ws_delim = " \t\r\n;=";
      ws.ws_alloc_die = imap4d_ws_alloc_die;
      if (mu_wordsplit (buffer, &ws, IMAP4D_WS_FLAGS))
	{
	  mu_error (_("%s failed: %s"), "mu_wordsplit",
		    mu_wordsplit_strerror (&ws));
	  return RESP_BAD; /* FIXME: a better error handling, maybe? */
	}

      len = strcspn (ws.ws_wordv[0], "/");
      if (mu_c_strcasecmp (ws.ws_wordv[0], "MESSAGE/RFC822") == 0)
        message_rfc822 = 1;
      else if (mu_c_strncasecmp (ws.ws_wordv[0], "TEXT", len) == 0)
        text_plain = 1;

      ws.ws_wordv[0][len++] = 0;
      p = ws.ws_wordv[0];
      io_send_qstring (p);
      io_sendf (" ");
      io_send_qstring (ws.ws_wordv[0] + len);

      /* body parameter parenthesized list: Content-type attributes */
      if (ws.ws_wordc > 1)
	{
	  int space = 0;
	  char *lvalue = NULL;
	  int i;
	  
	  io_sendf (" (");
	  for (i = 1; i < ws.ws_wordc; i++)
	    {
	      /* body parameter parenthesized list:
		 Content-type parameter list. */
	      if (lvalue)
		{
		  if (space)
		    io_sendf (" ");
		  io_send_qstring (lvalue);
		  lvalue = NULL;
		  space = 1;
		}
	      
	      switch (ws.ws_wordv[i][0])
		{
		case ';':
		  continue;
		  
		case '=':
		  if (++i < ws.ws_wordc)
		    {
		      io_sendf (" ");
		      io_send_qstring (ws.ws_wordv[i]);
		    }
		  break;
		  
		default:
		  lvalue = ws.ws_wordv[i];
		}
	    }
	  
	  if (lvalue)
	    {
	      if (space)
		io_sendf (" ");
	      io_send_qstring (lvalue);
	    }

	  io_sendf (")");
	}
      else
	io_sendf (" NIL");
      mu_wordsplit_free (&ws);
      free (buffer);
    }
  else
    {
      /* Default? If Content-Type is not present consider as text/plain.  */
      io_sendf ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\")");
      text_plain = 1;
    }
  
  /* body id: Content-ID. */
  fetch_send_header_value (header, MU_HEADER_CONTENT_ID, NULL, 1);
  /* body description: Content-Description. */
  fetch_send_header_value (header, MU_HEADER_CONTENT_DESCRIPTION, NULL, 1);

  /* body encoding: Content-Transfer-Encoding. */
  fetch_send_header_value (header, MU_HEADER_CONTENT_TRANSFER_ENCODING,
			   "7BIT", 1);

  /* body size RFC822 format.  */
  {
    size_t size = 0;
    mu_body_t body = NULL;
    mu_message_get_body (msg, &body);
    mu_body_size (body, &size);
    mu_body_lines (body, &blines);
    io_sendf (" %s", mu_umaxtostr (0, size + blines));
  }

  /* If the mime type was text.  */
  if (text_plain)
    {
      /* Add the line number of the body.  */
      io_sendf (" %s", mu_umaxtostr (0, blines));
    }
  else if (message_rfc822)
    {
      size_t lines = 0;
      mu_message_t emsg = NULL;
      mu_message_unencapsulate  (msg, &emsg, NULL);
      /* Add envelope structure of the encapsulated message.  */
      io_sendf (" (");
      fetch_envelope0 (emsg);
      io_sendf (")");
      /* Add body structure of the encapsulated message.  */
      io_sendf ("(");
      fetch_bodystructure0 (emsg, extension);
      io_sendf (")");
      /* Size in text lines of the encapsulated message.  */
      mu_message_lines (emsg, &lines);
      io_sendf (" %s", mu_umaxtostr (0, lines));
      mu_message_destroy (&emsg, NULL);
    }

  if (extension)
    {
      /* body MD5: Content-MD5.  */
      fetch_send_header_value (header, MU_HEADER_CONTENT_MD5, NULL, 1);

      /* body disposition: Content-Disposition.  */
      fetch_send_header_list (header, MU_HEADER_CONTENT_DISPOSITION, NULL, 1);

      /* body language: Content-Language.  */
      fetch_send_header_value (header, MU_HEADER_CONTENT_LANGUAGE, NULL, 1);
    }
  return RESP_OK;
}

/* The beef BODYSTRUCTURE.
   A parenthesized list that describes the [MIME-IMB] body structure of a
   message. Multiple parts are indicated by parenthesis nesting.  Instead of
   a body type as the first element of the parenthesized list there is a nested
   body.  The second element of the parenthesized list is the multipart
   subtype (mixed, digest, parallel, alternative, etc.).

   The extension data of a multipart body part are in the following order:
   body parameter parenthesized list:
   A parenthesized list of attribute/value pairs [e.g. ("foo" "bar" "baz"
   "rag") where "bar" is the value of "foo" and "rag" is the value of
   "baz"] as defined in [MIME-IMB].

   body disposition:
   A parenthesized list, consisting of a disposition type string followed by a
   parenthesized list of disposition attribute/value pairs.  The disposition
   type and attribute names will be defined in a future standards-track
   revision to [DISPOSITION].

   body language:
   A string or parenthesized list giving the body language value as defined
   in [LANGUAGE-TAGS].  */
static int
fetch_bodystructure0 (mu_message_t message, int extension)
{
  size_t nparts = 1;
  size_t i;
  int is_multipart = 0;

  mu_message_is_multipart (message, &is_multipart);
  if (is_multipart)
    {
      char *buffer = NULL;
      mu_header_t header = NULL;

      mu_message_get_num_parts (message, &nparts);

      /* Get all the sub messages.  */
      for (i = 1; i <= nparts; i++)
        {
          mu_message_t msg = NULL;
          mu_message_get_part (message, i, &msg);
          io_sendf ("(");
          fetch_bodystructure0 (msg, extension);
          io_sendf (")");
        } /* for () */

      mu_message_get_header (message, &header);

      /* The subtype.  */
      if (mu_header_aget_value (header, MU_HEADER_CONTENT_TYPE, &buffer) == 0)
	{
	  struct mu_wordsplit ws;
	  char *s;

	  ws.ws_delim = " \t\r\n;=";
	  ws.ws_alloc_die = imap4d_ws_alloc_die;
	  if (mu_wordsplit (buffer, &ws, IMAP4D_WS_FLAGS))
	    {
	      mu_error (_("%s failed: %s"), "mu_wordsplit",
			mu_wordsplit_strerror (&ws));
	      return RESP_BAD; /* FIXME: a better error handling, maybe? */
	    }

	  s = strchr (ws.ws_wordv[0], '/');
	  if (s)
	    s++;
	  io_sendf (" ");
	  io_send_qstring (s);

	  /* The extension data for multipart. */
	  if (extension)
	    {
	      int space = 0;
	      char *lvalue = NULL;
	      
	      io_sendf (" (");
	      for (i = 1; i < ws.ws_wordc; i++)
		{
		  /* body parameter parenthesized list:
		     Content-type parameter list. */
		  if (lvalue)
		    {
		      if (space)
			io_sendf (" ");
		      io_send_qstring (lvalue);
		      lvalue = NULL;
		      space = 1;
		    }

		  switch (ws.ws_wordv[i][0])
		    {
		    case ';':
		      continue;
		      
		    case '=':
		      if (++i < ws.ws_wordc)
			{
			  io_sendf (" ");
			  io_send_qstring (ws.ws_wordv[i]);
			}
		      break;
		      
		    default:
		      lvalue = ws.ws_wordv[i];
		    }
		}
	      if (lvalue)
		{
		  if (space)
		    io_sendf (" ");
		  io_send_qstring (lvalue);
		}
	      io_sendf (")");
	    }
	  else
	    io_sendf (" NIL");
	  mu_wordsplit_free (&ws);
          free (buffer);
	}
      else
	/* No content-type header */
	io_sendf (" NIL");

      /* body disposition: Content-Disposition.  */
      fetch_send_header_list (header, MU_HEADER_CONTENT_DISPOSITION,
			      NULL, 1);
      /* body language: Content-Language.  */
      fetch_send_header_list (header, MU_HEADER_CONTENT_LANGUAGE,
			      NULL, 1);
    }
  else
    bodystructure (message, extension);
  return RESP_OK;
}

static void
set_seen (struct fetch_function_closure *ffc,
	  struct fetch_runtime_closure *frt)
{
  if (!ffc->peek)
    {
      mu_attribute_t attr = NULL;
      mu_message_get_attribute (frt->msg, &attr);
      if (!mu_attribute_is_read (attr))
	{
	  io_sendf ("FLAGS (\\Seen) ");
	  mu_attribute_set_read (attr);
	}
    }
}

static mu_message_t 
fetch_get_part (struct fetch_function_closure *ffc,
		struct fetch_runtime_closure *frt)
{
  mu_message_t msg = frt->msg;
  size_t i;

  for (i = 0; i < ffc->nset; i++)
    if (mu_message_get_part (msg, ffc->section_part[i], &msg))
      return NULL;
  return msg;
}

/* FIXME: This is a KLUDGE.

   There is so far no way to unref the MU elements being used to construct
   a certain entity when this entity itself is being destroyed.  In particular,
   retrieving a nested message/rfc822 part involves creating several messages
   while unencapsulating, which messages should be unreferenced when the
   topmost one is destroyed.

   A temporary solution used here is to keep a list of such messages and
   unreference them together with the topmost one when no longer needed.
   A message is added to the list by frt_register_message().  All messages
   in the list are unreferenced by calling frt_unregister_messages().

   The proper solution is of course providing a way for mu_message_t (and
   other MU objects) to unreference its parent elements.  This should be fixed
   in later releases.
 */

static void
_unref_message_item (void *data)
{
  mu_message_unref ((mu_message_t)data);
}

static int
frt_register_message (struct fetch_runtime_closure *frt, mu_message_t msg)
{
  if (!frt->msglist)
    {
      int rc = mu_list_create (&frt->msglist);
      if (rc)
	return rc;
      mu_list_set_destroy_item (frt->msglist, _unref_message_item);
    }
  return mu_list_append (frt->msglist, msg);
}

static void
frt_unregister_messages (struct fetch_runtime_closure *frt)
{
  mu_list_clear (frt->msglist);
}

static mu_message_t 
fetch_get_part_rfc822 (struct fetch_function_closure *ffc,
		       struct fetch_runtime_closure *frt)
{
  mu_message_t msg = frt->msg, retmsg = NULL;
  size_t i;
  mu_header_t header;
  const char *hval;
  
  if (ffc->nset == 0)
    {
      mu_message_ref (msg);
      return msg;
    }
  
  for (i = 0; i < ffc->nset; i++)
    {
      if (mu_message_get_part (msg, ffc->section_part[i], &msg))
	return NULL;

      if (mu_message_get_header (msg, &header))
	return NULL;
  
      if (mu_header_sget_value (header, MU_HEADER_CONTENT_TYPE, &hval) == 0)
	{
	  struct mu_wordsplit ws;
	  int rc;
      
	  ws.ws_delim = " \t\r\n;=";
	  ws.ws_alloc_die = imap4d_ws_alloc_die;
	  if (mu_wordsplit (hval, &ws, IMAP4D_WS_FLAGS))
	    {
	      mu_error (_("%s failed: %s"), "mu_wordsplit",
			mu_wordsplit_strerror (&ws));
	      return NULL;
	    }

	  rc = mu_c_strcasecmp (ws.ws_wordv[0], "MESSAGE/RFC822");
	  mu_wordsplit_free (&ws);

	  if (rc == 0)
	    {
	      rc = mu_message_unencapsulate  (msg, &retmsg, NULL);
	      if (rc)
		{
		  mu_error (_("%s failed: %s"), "mu_message_unencapsulate",
			    mu_strerror (rc));
		  return NULL;
		}
	      if (frt_register_message (frt, retmsg))
		{
		  frt_unregister_messages (frt);
		  return NULL;
		}
	      msg = retmsg;
	    }
	}
    }
  
  return retmsg;
}

static void
fetch_send_section_part (struct fetch_function_closure *ffc,
                         const char *suffix, int close_bracket)
{
  int i;
  
  io_sendf ("BODY[");
  for (i = 0; i < ffc->nset; i++)
    {
      if (i)
	io_sendf (".");
      io_sendf ("%lu",  (unsigned long) ffc->section_part[i]);
    }
  if (suffix)
    {
      if (i)
	io_sendf (".");
      io_sendf ("%s", suffix);
    }
  if (close_bracket)
    io_sendf ("]");
}

static int
fetch_io (mu_stream_t stream, size_t start, size_t size, size_t max)
{
  mu_stream_t rfc = NULL;
  
  size_t n = 0;

  mu_filter_create (&rfc, stream, "CRLF", MU_FILTER_ENCODE,
		    MU_STREAM_READ|MU_STREAM_SEEK);
  
  if (start == 0 && size == (size_t) -1)
    {
      int rc;
      
      rc = mu_stream_seek (rfc, 0, MU_SEEK_SET, NULL);
      if (rc)
	{
	  mu_error ("seek error: %s", mu_stream_strerror (stream, rc));
	  return RESP_BAD;
	}
      if (max)
	{
	  io_sendf (" {%lu}\n", (unsigned long) max);
	  io_copy_out (rfc, max);
	  /* FIXME: Make sure exactly max bytes were sent */
	}
      else
	io_sendf (" \"\"");
    }
  else if (start > max)
    {
      io_sendf ("<%lu>", (unsigned long) start);
      io_sendf (" \"\"");
    }
  else
    {
      int rc;
      char *buffer, *p;
      size_t total = 0;

      if (size > max)
	size = max;
      if (size + 2 < size) /* Check for integer overflow */
	{
	  mu_stream_destroy (&rfc);
	  return RESP_BAD;
	}
      
      p = buffer = mu_alloc (size + 1);

      rc = mu_stream_seek (rfc, start, MU_SEEK_SET, NULL);
      if (rc)
	{
	  mu_error ("seek error: %s", mu_stream_strerror (rfc, rc));
	  free (buffer);
	  mu_stream_destroy (&rfc);
	  return RESP_BAD;
	}

      while (total < size
	     && mu_stream_read (rfc, p, size - total, &n) == 0
	     && n > 0)
	{
	  total += n;
	  p += n;
	}
      *p = 0;
      io_sendf ("<%lu>", (unsigned long) start);
      if (total)
	{
	  io_sendf (" {%lu}\n", (unsigned long) total);
	  io_send_bytes (buffer, total);
	}
      else
	io_sendf (" \"\"");
      free (buffer);
    }
  mu_stream_destroy (&rfc);
  return RESP_OK;
}


/* Runtime functions */
static int
_frt_uid (struct fetch_function_closure *ffc,
	  struct fetch_runtime_closure *frt)
{
  size_t uid = 0;

  mu_message_get_uid (frt->msg, &uid);
  io_sendf ("%s %s", ffc->name, mu_umaxtostr (0, uid));
  return RESP_OK;
}

static int
_frt_envelope (struct fetch_function_closure *ffc,
	       struct fetch_runtime_closure *frt)
{
  io_sendf ("%s (", ffc->name);
  fetch_envelope0 (frt->msg);
  io_sendf (")");
  return RESP_OK;
}

static int
_frt_flags (struct fetch_function_closure *ffc,
	    struct fetch_runtime_closure *frt)
{
  mu_attribute_t attr = NULL;

  mu_message_get_attribute (frt->msg, &attr);
  io_sendf ("%s (", ffc->name);
  util_print_flags (attr);
  io_sendf (")");
  return 0;
}

/* INTERNALDATE   The internal date of the message.
   Format:

   date_time       ::= <"> date_day_fixed "-" date_month "-" date_year
   SPACE time SPACE zone <">

   date_day        ::= 1*2digit
   ;; Day of month

   date_day_fixed  ::= (SPACE digit) / 2digit
   ;; Fixed-format version of date_day

   date_month      ::= "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" /
   "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"

   date_text       ::= date_day "-" date_month "-" date_year

   date_year       ::= 4digit

   time            ::= 2digit ":" 2digit ":" 2digit
   ;; Hours minutes seconds

   zone            ::= ("+" / "-") 4digit
   ;; Signed four-digit value of hhmm representing
   ;; hours and minutes west of Greenwich (that is,
   ;; (the amount that the given time differs from
   ;; Universal Time).  Subtracting the timezone
   ;; from the given time will give the UT form.
   ;; The Universal Time zone is "+0000".  */
static int
_frt_internaldate (struct fetch_function_closure *ffc,
		   struct fetch_runtime_closure *frt)
{
  const char *date;
  mu_envelope_t env = NULL;
  struct tm tm, *tmp = NULL;
  struct mu_timezone tz;
  
  mu_message_get_envelope (frt->msg, &env);
  if (mu_envelope_sget_date (env, &date) == 0
      && mu_scan_datetime (date, MU_DATETIME_FROM, &tm, &tz, NULL) == 0)
    {
      tmp = &tm;
    }
  else
    {
      time_t t;
      struct timeval stv;
      struct timezone stz;
      
      gettimeofday (&stv, &stz);
      t = stv.tv_sec;
      tz.utc_offset = - stz.tz_minuteswest;
      tmp = localtime (&t);
    }
  io_sendf ("%s", ffc->name);
  mu_c_streamftime (iostream, " \"%d-%b-%Y %H:%M:%S %z\"", tmp, &tz);
  return 0;
}

static int
_frt_bodystructure (struct fetch_function_closure *ffc,
		    struct fetch_runtime_closure *frt)
{
  io_sendf ("%s (", ffc->name);
  fetch_bodystructure0 (frt->msg, 1); /* 1 means with extension data.  */
  io_sendf (")");
  return RESP_OK;
}

static int
_frt_bodystructure0 (struct fetch_function_closure *ffc,
		     struct fetch_runtime_closure *frt)
{
  io_sendf ("%s (", ffc->name);
  fetch_bodystructure0 (frt->msg, 0);
  io_sendf (")");
  return RESP_OK;
}

/* BODY[] */
static int
_frt_body (struct fetch_function_closure *ffc,
	   struct fetch_runtime_closure *frt)
{
  mu_message_t msg = frt->msg;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  int rc;
  
  set_seen (ffc, frt);
  if (ffc->name)
    io_sendf ("%s", ffc->name);
  else
    fetch_send_section_part (ffc, NULL, 1);
  mu_message_get_streamref (msg, &stream);
  mu_message_size (msg, &size);
  mu_message_lines (msg, &lines);
  rc = fetch_io (stream, ffc->start, ffc->size, size + lines);
  mu_stream_destroy (&stream);
  return rc;
}

/* BODY[N] */
static int
_frt_body_n (struct fetch_function_closure *ffc,
	     struct fetch_runtime_closure *frt)
{
  mu_message_t msg;
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  int rc;
  
  set_seen (ffc, frt);
  if (ffc->name)
    io_sendf ("%s",  ffc->name);
  else
    fetch_send_section_part (ffc, ffc->section_tag, 1);
  msg = fetch_get_part (ffc, frt);
  if (!msg)
    {
      io_sendf (" NIL");
      return RESP_OK;
    }

  mu_message_get_body (msg, &body);
  mu_body_size (body, &size);
  mu_body_lines (body, &lines);
  mu_body_get_streamref (body, &stream);
  rc = fetch_io (stream, ffc->start, ffc->size, size + lines);
  mu_stream_destroy (&stream);

  return rc;
}

static int
_frt_body_text (struct fetch_function_closure *ffc,
		struct fetch_runtime_closure *frt)
{
  mu_message_t msg;
  mu_body_t body = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  int rc;
  
  set_seen (ffc, frt);
  if (ffc->name)
    io_sendf ("%s",  ffc->name);
  else
    fetch_send_section_part (ffc, ffc->section_tag, 1);
  msg = fetch_get_part_rfc822 (ffc, frt);
  if (!msg)
    {
      io_sendf (" NIL");
      return RESP_OK;
    }

  mu_message_get_body (msg, &body);
  mu_body_size (body, &size);
  mu_body_lines (body, &lines);
  mu_body_get_streamref (body, &stream);
  rc = fetch_io (stream, ffc->start, ffc->size, size + lines);
  mu_stream_destroy (&stream);
  frt_unregister_messages (frt);
  return rc;
}

static int
_frt_size (struct fetch_function_closure *ffc,
	   struct fetch_runtime_closure *frt)
{
  size_t size = 0;
  size_t lines = 0;
  
  mu_message_size (frt->msg, &size);
  mu_message_lines (frt->msg, &lines);
  io_sendf ("%s %lu", ffc->name, (unsigned long) (size + lines));
  return RESP_OK;
}

static int
_frt_header (struct fetch_function_closure *ffc,
	     struct fetch_runtime_closure *frt)
{
  mu_message_t msg;
  mu_header_t header = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  int rc;
  
  set_seen (ffc, frt);
  if (ffc->name)
    io_sendf ("%s",  ffc->name);
  else
    fetch_send_section_part (ffc, ffc->section_tag, 1);

  msg = fetch_get_part_rfc822 (ffc, frt);
  if (!msg)
    {
      io_sendf (" NIL");
      return RESP_OK;
    }
  mu_message_get_header (msg, &header);
  mu_header_size (header, &size);
  mu_header_lines (header, &lines);
  mu_header_get_streamref (header, &stream);
  rc = fetch_io (stream, ffc->start, ffc->size, size + lines);
  mu_stream_destroy (&stream);
  frt_unregister_messages (frt);
  return rc;
}

static int
_frt_mime (struct fetch_function_closure *ffc,
	     struct fetch_runtime_closure *frt)
{
  mu_message_t msg;
  mu_header_t header = NULL;
  mu_stream_t stream = NULL;
  size_t size = 0, lines = 0;
  int rc;
  
  set_seen (ffc, frt);
  if (ffc->name)
    io_sendf ("%s",  ffc->name);
  else
    fetch_send_section_part (ffc, ffc->section_tag, 1);

  msg = fetch_get_part (ffc, frt);
  if (!msg)
    {
      io_sendf (" NIL");
      return RESP_OK;
    }
  mu_message_get_header (msg, &header);
  mu_header_size (header, &size);
  mu_header_lines (header, &lines);
  mu_header_get_streamref (header, &stream);
  rc = fetch_io (stream, ffc->start, ffc->size, size + lines);
  mu_stream_destroy (&stream);

  return rc;
}

static int
_send_header_name (void *item, void *data)
{
  int *pf = data;
  if (*pf)
    io_sendf (" ");
  else
    *pf = 1;
  io_sendf ("%s", (char*) item);
  return 0;
}

static int
count_nl (const char *str)
{
  int n = 0;
  for (;(str = strchr (str, '\n')); str++)
    n++;
  return n;
}

static int
_frt_header_fields (struct fetch_function_closure *ffc,
		    struct fetch_runtime_closure *frt)
{
  int status;
  mu_message_t msg;
  mu_off_t size = 0;
  size_t lines = 0;
  mu_stream_t stream;
  mu_header_t header;
  mu_iterator_t itr;
  
  set_seen (ffc, frt);

  fetch_send_section_part (ffc, "HEADER.FIELDS", 0);
  if (ffc->not)
    io_sendf (".NOT");
  io_sendf (" (");
  status = 0;
  mu_list_foreach (ffc->headers, _send_header_name, &status);
  io_sendf (")]");
  
  msg = fetch_get_part_rfc822 (ffc, frt);
  if (!msg)
    {
      io_sendf (" NIL");
      return RESP_OK;
    }

  /* Collect headers: */
  if (mu_message_get_header (msg, &header)
      || mu_header_get_iterator (header, &itr))
    {
      frt_unregister_messages (frt);
      io_sendf (" NIL");
      return RESP_OK;
    }

  status = mu_memory_stream_create (&stream, 0);
  if (status != 0)
    imap4d_bye (ERR_NO_MEM);

  for (mu_iterator_first (itr);
       !mu_iterator_is_done (itr); mu_iterator_next (itr))
    {
      const char *hf;
      char *hv;
      const char *item;
      
      mu_iterator_current_kv (itr, (const void **)&hf, (void **)&hv);
      status = mu_list_locate (ffc->headers, (void *)hf, (void**) &item) == 0;
      if (ffc->not)
	{
	  status = !status;
	  item = hf;
	}
      
      if (status)
	{
	  mu_stream_printf (stream, "%s: %s\n", item, hv);
	  lines += 1 + count_nl (hv);
	}
    }
  mu_iterator_destroy (&itr);
  mu_stream_write (stream, "\n", 1, NULL);
  lines++;
  
  /* Output collected data */
  mu_stream_size (stream, &size);
  mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  status = fetch_io (stream, ffc->start, ffc->size, size + lines);
  mu_stream_destroy (&stream);
  frt_unregister_messages (frt);
  
  return status;
}


static void
ffc_init (struct fetch_function_closure *ffc)
{
  memset(ffc, 0, sizeof *ffc);
  ffc->start = 0;
  ffc->size = (size_t) -1;
}

static void
_free_ffc (void *item)
{
  struct fetch_function_closure *ffc = item;
  mu_list_destroy (&ffc->headers);
  free (ffc);
}

static int
_do_fetch (void *item, void *data)
{
  struct fetch_function_closure *ffc = item;
  struct fetch_runtime_closure *frt = data;
  if (frt->eltno++)
    io_sendf (" ");
  return ffc->fun (ffc, frt);
}

static void
append_ffc (struct fetch_parse_closure *p, struct fetch_function_closure *ffc)
{
  struct fetch_function_closure *new_ffc = mu_alloc (sizeof (*new_ffc));
  *new_ffc = *ffc;
  mu_list_append (p->fnlist, new_ffc);
}

static void
append_simple_function (struct fetch_parse_closure *p, const char *name,
			fetch_function_t fun)
{
  struct fetch_function_closure ffc;
  ffc_init (&ffc);
  ffc.fun = fun;
  ffc.name = name;
  append_ffc (p, &ffc);
}


static struct fetch_macro
{
  char *macro;
  char *exp;
} fetch_macro_tab[] = {
  { "ALL",  "FLAGS INTERNALDATE RFC822.SIZE ENVELOPE" },
  { "FULL", "FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY" },
  { "FAST", "FLAGS INTERNALDATE RFC822.SIZE" },
  { NULL }
};

static char *
find_macro (const char *name)
{
  int i;
  for (i = 0; fetch_macro_tab[i].macro; i++)
    if (mu_c_strcasecmp (fetch_macro_tab[i].macro, name) == 0)
      return fetch_macro_tab[i].exp;
  return NULL;
}


struct fetch_att_tab
{
  char *name;
  fetch_function_t fun;
};

static struct fetch_att_tab fetch_att_tab[] = {
  { "ENVELOPE", _frt_envelope },
  { "FLAGS", _frt_flags },
  { "INTERNALDATE", _frt_internaldate },
  { "UID", _frt_uid },
  { NULL }
};

static struct fetch_att_tab *
find_fetch_att_tab (char *name)
{
  struct fetch_att_tab *p;
  for (p = fetch_att_tab; p->name; p++)
    if (mu_c_strcasecmp (p->name, name) == 0)
      return p;
  return NULL;
}

/*
fetch-att       = "ENVELOPE" / "FLAGS" / "INTERNALDATE" /
                  "RFC822" [".HEADER" / ".SIZE" / ".TEXT"] /
                  "BODY" ["STRUCTURE"] / "UID" /
                  "BODY" section ["<" number "." nz-number ">"] /
                  "BODY.PEEK" section ["<" number "." nz-number ">"]

*/

/*  "RFC822" [".HEADER" / ".SIZE" / ".TEXT"]  */
static void
parse_fetch_rfc822 (imap4d_parsebuf_t p)
{
  struct fetch_function_closure ffc;
  ffc_init (&ffc);
  ffc.name = "RFC822";
  imap4d_parsebuf_next (p, 0);
  if (p->token == NULL || p->token[0] == ')') 
    {
      /* Equivalent to BODY[]. */
      ffc.fun = _frt_body;
    }
  else if (p->token[0] == '.')
    {
      imap4d_parsebuf_next (p, 1);
      if (mu_c_strcasecmp (p->token, "HEADER") == 0)
	{
	  /* RFC822.HEADER
	     Equivalent to BODY[HEADER].  Note that this did not result in
	     \Seen being set, because RFC822.HEADER response data occurs as
	     a result of a FETCH of RFC822.HEADER.  BODY[HEADER] response
	     data occurs as a result of a FETCH of BODY[HEADER] (which sets
	     \Seen) or BODY.PEEK[HEADER] (which does not set \Seen). */

	  ffc.name = "RFC822.HEADER";
	  ffc.fun = _frt_header;
	  ffc.peek = 1;
	  imap4d_parsebuf_next (p, 0);
	}
      else if (mu_c_strcasecmp (p->token, "SIZE") == 0)
	{
	  /* A number expressing the [RFC-2822] size of the message. */
	  ffc.name = "RFC822.SIZE";
	  ffc.fun = _frt_size;
	  imap4d_parsebuf_next (p, 0);
	}
      else if (mu_c_strcasecmp (p->token, "TEXT") == 0)
	{
	  /* RFC822.TEXT
	     Equivalent to BODY[TEXT]. */
	  ffc.name = "RFC822.TEXT";
	  ffc.fun = _frt_body_text;
	  imap4d_parsebuf_next (p, 0);
	}
      else
	imap4d_parsebuf_exit (p, "Syntax error after RFC822.");
    }
  else
    imap4d_parsebuf_exit (p, "Syntax error after RFC822");
  append_ffc (imap4d_parsebuf_data (p), &ffc);
}

static int
_header_cmp (const void *a, const void *b)
{
  return mu_c_strcasecmp ((char*)a, (char*)b);
}

/*
header-fld-name = astring

header-list     = "(" header-fld-name *(SP header-fld-name) ")"
*/
static void
parse_header_list (imap4d_parsebuf_t p, struct fetch_function_closure *ffc)
{
  if (!(p->token && p->token[0] == '('))
    imap4d_parsebuf_exit (p, "Syntax error: expected (");
  mu_list_create (&ffc->headers);
  mu_list_set_comparator (ffc->headers, _header_cmp);
  for (imap4d_parsebuf_next (p, 1); p->token[0] != ')'; imap4d_parsebuf_next (p, 1))
    {
      if (p->token[1] == 0 && strchr ("()[]<>.", p->token[0]))
	imap4d_parsebuf_exit (p, "Syntax error: unexpected delimiter");
      mu_list_append (ffc->headers, p->token);
    }
  imap4d_parsebuf_next (p, 1);
}

/*
section-msgtext = "HEADER" / "HEADER.FIELDS" [".NOT"] SP header-list /
                  "TEXT"
                    ; top-level or MESSAGE/RFC822 part
section-text    = section-msgtext / "MIME"
                    ; text other than actual body part (headers, etc.)
*/  
static int
parse_section_text (imap4d_parsebuf_t p, struct fetch_function_closure *ffc,
		    int allow_mime)
{
  if (mu_c_strcasecmp (p->token, "HEADER") == 0)
    {
      /* "HEADER" / "HEADER.FIELDS" [".NOT"] SP header-list  */
      imap4d_parsebuf_next (p, 1);
      if (p->token[0] == '.')
	{
	  imap4d_parsebuf_next (p, 1);
	  if (mu_c_strcasecmp (p->token, "FIELDS"))
	    imap4d_parsebuf_exit (p, "Expected FIELDS");
	  ffc->fun = _frt_header_fields;
	  imap4d_parsebuf_next (p, 1);
	  if (p->token[0] == '.')
	    {
	      imap4d_parsebuf_next (p, 1);
	      if (mu_c_strcasecmp (p->token, "NOT") == 0)
		{
		  ffc->not = 1;
		  imap4d_parsebuf_next (p, 1);
		}
	      else
		imap4d_parsebuf_exit (p, "Expected NOT");
	    }
	  parse_header_list (p, ffc);
	}
      else
	{
	  ffc->fun = _frt_header;
	  ffc->section_tag = "HEADER";
	}
    }
  else if (mu_c_strcasecmp (p->token, "TEXT") == 0)
    {
      imap4d_parsebuf_next (p, 1);
      ffc->fun = _frt_body_text;
      ffc->section_tag = "TEXT";
    }
  else if (allow_mime && mu_c_strcasecmp (p->token, "MIME") == 0)
    {
      imap4d_parsebuf_next (p, 1);
      ffc->fun = _frt_mime;
      ffc->section_tag = "MIME";
    }
  else
    return 1;
 return 0;
}

static size_t
parsebuf_get_number (imap4d_parsebuf_t p)
{
  char *cp;
  unsigned long n = strtoul (p->token, &cp, 10);

  if (*cp)
    imap4d_parsebuf_exit (p, "Syntax error: expected number");
  return n;
}
    
/*
section-part    = nz-number *("." nz-number)
                    ; body part nesting
*/  
static void
parse_section_part (imap4d_parsebuf_t p, struct fetch_function_closure *ffc)
{
  size_t *parts;
  size_t nmax = 0;
  size_t ncur = 0;

  for (;;)
    {
      char *cp;
      size_t n = parsebuf_get_number (p);
      if (ncur == nmax)
	{
	  if (nmax == 0)
	    {
	      nmax = 16;
	      parts = calloc (nmax, sizeof (parts[0]));
	    }
	  else
	    {
	      nmax *= 2;
	      parts = realloc (parts, nmax * sizeof (parts[0]));
	    }
	  if (!parts)
	    imap4d_bye (ERR_NO_MEM);
	}
      parts[ncur++] = n;

      imap4d_parsebuf_next (p, 1);
      
      if (p->token[0] == '.'
	  && (cp = imap4d_parsebuf_peek (p))
	  && mu_isdigit (*cp))
	imap4d_parsebuf_next (p, 1);
      else
	break;
    }
  ffc->section_part = parts;
  ffc->nset = ncur;
}
  
/*
section         = "[" [section-spec] "]"
section-spec    = section-msgtext / (section-part ["." section-text])
*/  
static int
parse_section (imap4d_parsebuf_t p, struct fetch_function_closure *ffc)
{
  if (p->token[0] != '[')
    return 1;
  ffc_init (ffc);
  ffc->name = NULL;
  ffc->fun = _frt_body_text;
  imap4d_parsebuf_next (p, 1);
  if (parse_section_text (p, ffc, 0))
    {
      if (p->token[0] == ']')
	ffc->fun = _frt_body;
      else if (mu_isdigit (p->token[0]))
	{
	  parse_section_part (p, ffc);
	  if (p->token[0] == '.')
	    {
	      imap4d_parsebuf_next (p, 1);
	      parse_section_text (p, ffc, 1);
	    }
	  else
	    ffc->fun = _frt_body_n;
	}
      else
	imap4d_parsebuf_exit (p, "Syntax error");
    }
  if (p->token[0] != ']')
    imap4d_parsebuf_exit (p, "Syntax error: missing ]");
  imap4d_parsebuf_next (p, 0);
  return 0;
}

static void
parse_substring (imap4d_parsebuf_t p, struct fetch_function_closure *ffc)
{
  if (p->token && p->token[0] == '<')
    {
      imap4d_parsebuf_next (p, 1);
      ffc->start = parsebuf_get_number (p);
      imap4d_parsebuf_next (p, 1);
      if (p->token[0] != '.')
	imap4d_parsebuf_exit (p, "Syntax error: expected .");
      imap4d_parsebuf_next (p, 1);
      ffc->size = parsebuf_get_number (p);
      imap4d_parsebuf_next (p, 1);
      if (p->token[0] != '>')
	imap4d_parsebuf_exit (p, "Syntax error: expected >");
      imap4d_parsebuf_next (p, 0);
    }
}
	
/* section ["<" number "." nz-number ">"]  */
static int
parse_body_args (imap4d_parsebuf_t p, int peek)
{
  struct fetch_function_closure ffc;
  if (parse_section (p, &ffc) == 0)
    {
      parse_substring (p, &ffc);
      ffc.peek = peek;
      append_ffc (imap4d_parsebuf_data (p), &ffc);
      return 0;
    }
  return 1;
}

static void
parse_body_peek (imap4d_parsebuf_t p)
{
  imap4d_parsebuf_next (p, 1);
  if (mu_c_strcasecmp (p->token, "PEEK") == 0)
    {
      imap4d_parsebuf_next (p, 1);
      if (parse_body_args (p, 1))
	imap4d_parsebuf_exit (p, "Syntax error");
    }
  else
    imap4d_parsebuf_exit (p, "Syntax error: expected PEEK");
}

/*  "BODY" ["STRUCTURE"] / 
    "BODY" section ["<" number "." nz-number ">"] /
    "BODY.PEEK" section ["<" number "." nz-number ">"] */
static void
parse_fetch_body (imap4d_parsebuf_t p)
{
  if (imap4d_parsebuf_next (p, 0) == NULL || p->token[0] == ')')
    append_simple_function (imap4d_parsebuf_data (p),
			    "BODY", _frt_bodystructure0);
  else if (p->token[0] == '.')
    parse_body_peek (p);
  else if (mu_c_strcasecmp (p->token, "STRUCTURE") == 0)
    {
      /* For compatibility with previous versions */
      append_simple_function (imap4d_parsebuf_data (p),
			      "BODYSTRUCTURE", _frt_bodystructure);
      imap4d_parsebuf_next (p, 0);
    }
  else if (parse_body_args (p, 0))
    append_simple_function (imap4d_parsebuf_data (p),
			    "BODY", _frt_bodystructure0);
}

static int
parse_fetch_att (imap4d_parsebuf_t p)
{
  struct fetch_att_tab *ent;
  struct fetch_parse_closure *pclos = imap4d_parsebuf_data (p);
  
  ent = find_fetch_att_tab (p->token);
  if (ent)
    {
      if (!(ent->fun == _frt_uid && pclos->isuid))
	append_simple_function (pclos, ent->name, ent->fun);
      imap4d_parsebuf_next (p, 0);
    }
  else if (mu_c_strcasecmp (p->token, "RFC822") == 0)
    parse_fetch_rfc822 (p);
  else if (mu_c_strcasecmp (p->token, "BODY") == 0)
    parse_fetch_body (p);
  else if (mu_c_strcasecmp (p->token, "BODYSTRUCTURE") == 0)
    {
      append_simple_function (pclos, "BODYSTRUCTURE", _frt_bodystructure);
      imap4d_parsebuf_next (p, 0);
    }
  else
    return 1;
  return 0;
}

/* fetch-att *(SP fetch-att) */
static void
parse_fetch_att_list (imap4d_parsebuf_t p)
{
  while (p->token && parse_fetch_att (p) == 0)
    ;
}

/* "ALL" / "FULL" / "FAST" / fetch-att / "(" */
static void
parse_macro (imap4d_parsebuf_t p)
{  
  char *exp;
  
  imap4d_parsebuf_next (p, 1);
  if (p->token[0] == '(')
    {
      imap4d_parsebuf_next (p, 1);
      parse_fetch_att_list (p);
      if (!(p->token && p->token[0] == ')'))
	imap4d_parsebuf_exit (p, "Unknown token or missing closing parenthesis");
    }
  else if ((exp = find_macro (p->token))) 
    {
      imap4d_tokbuf_t save_tok = p->tok;
      int save_arg = p->arg;
      p->tok = imap4d_tokbuf_from_string (exp);
      p->arg = 0;
      imap4d_parsebuf_next (p, 1);
      parse_fetch_att_list (p);
      imap4d_tokbuf_destroy (&p->tok);
  
      p->arg = save_arg;
      p->tok = save_tok;

      if (imap4d_parsebuf_peek (p))
	imap4d_parsebuf_exit (p, "Too many arguments");
    }     
  else
    {
      parse_fetch_att (p);
      if (p->token)
	imap4d_parsebuf_exit (p, "Too many arguments");
    }
}
    

static int
fetch_thunk (imap4d_parsebuf_t pb)
{
  int status;
  char *mstr;
  char *end;
  struct fetch_parse_closure *pclos = imap4d_parsebuf_data (pb);
  
  mstr = imap4d_parsebuf_next (pb, 1);

  status = mu_msgset_create (&pclos->msgset, mbox, MU_MSGSET_NUM);
  if (status)
    imap4d_parsebuf_exit (pb, "Software error");
  
  /* Parse sequence numbers. */
  status = mu_msgset_parse_imap (pclos->msgset,
				 pclos->isuid ? MU_MSGSET_UID : MU_MSGSET_NUM,
				 mstr, &end);
  if (status)
    imap4d_parsebuf_exit (pb, "Failed to parse message set");
  
  /* Compile the expression */

  /* Server implementations MUST implicitly
     include the UID message data item as part of any FETCH
     response caused by a UID command, regardless of whether
     a UID was specified as a message data item to the FETCH. */
  if (pclos->isuid)
    append_simple_function (pclos, "UID", _frt_uid);

  parse_macro (pb);
  return RESP_OK;
}

int
_fetch_from_message (size_t msgno, mu_message_t msg, void *data)
{
  int rc = 0;
  struct fetch_runtime_closure *frc = data;

  frc->msgno = msgno;
  frc->msg = msg;

  io_sendf ("* %lu FETCH (", (unsigned long) msgno);
  frc->eltno = 0;
  rc = mu_list_foreach (frc->fnlist, _do_fetch, frc);
  io_sendf (")\n");

  return rc;
}

/* Where the real implementation is.  It is here since UID command also
   calls FETCH.  */
int
imap4d_fetch0 (imap4d_tokbuf_t tok, int isuid, char **err_text)
{
  int rc;
  struct fetch_parse_closure pclos;
  
  if (imap4d_tokbuf_argc (tok) - (IMAP4_ARG_1 + isuid) < 2)
    {
      *err_text = "Invalid arguments";
      return 1;
    }

  memset (&pclos, 0, sizeof (pclos));
  pclos.isuid = isuid;
  mu_list_create (&pclos.fnlist);
  mu_list_set_destroy_item (pclos.fnlist, _free_ffc);
  
  rc = imap4d_with_parsebuf (tok, IMAP4_ARG_1 + isuid,
			     ".[]<>",
			     fetch_thunk, &pclos,
			     err_text);

  if (rc == RESP_OK)
    {
      struct fetch_runtime_closure frc;

      memset (&frc, 0, sizeof (frc));
      frc.fnlist = pclos.fnlist;
      /* Prepare status code. It will be replaced if an error occurs in the
	 loop below */
      frc.err_text = "Completed";

      mu_msgset_foreach_message (pclos.msgset, _fetch_from_message, &frc);
      mu_list_destroy (&frc.msglist);
    }
  
  mu_list_destroy (&pclos.fnlist);
  mu_msgset_free (pclos.msgset);
  return rc;
}


/*
6.4.5.  FETCH Command

   Arguments:  message set
               message data item names

   Responses:  untagged responses: FETCH

   Result:     OK - fetch completed
               NO - fetch error: can't fetch that data
               BAD - command unknown or arguments invalid
*/

/* The FETCH command retrieves data associated with a message in the
   mailbox.  The data items to be fetched can be either a single atom
   or a parenthesized list.  */
int
imap4d_fetch (struct imap4d_session *session,
              struct imap4d_command *command, imap4d_tokbuf_t tok)
{
  int rc;
  char *err_text = "Completed";
  int xlev = set_xscript_level (MU_XSCRIPT_PAYLOAD);
  rc = imap4d_fetch0 (tok, 0, &err_text);
  set_xscript_level (xlev);
  return io_completion_response (command, rc, "%s", err_text);
}
