/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/errno.h>

int
rfc2047_decode (const char *tocode, const char *input, char **ptostr)
{
  int status = 0;
  char *tmpcopy, *fromstr;
  char *start_position = NULL;
  char *buffer;
  size_t bufsize;
  size_t bufpos;
  size_t run_count = 0;
  
  if (!tocode || !input || !ptostr)
    return EINVAL;

  /* Prepare a temporary copy of the input string (strtok_r is
     going to modify it. */
  tmpcopy = strdup (input);
  if (!tmpcopy)
    return ENOMEM;
  fromstr = tmpcopy;

  /* Allocate the buffer. It is assumed that encoded string is always
     longer than it's decoded variant, so it's safe to use its length
     as the first estimate */
  bufsize = strlen (fromstr) + 1;
  buffer = malloc (bufsize);
  if (buffer == NULL)
    {
      free (tmpcopy);
      return ENOMEM;
    }
  bufpos = 0;
  
  while (*fromstr)
    {
      if (strncmp (fromstr, "=?", 2) == 0)
	{
	  char *fromcode = NULL;
	  char *encoding_type = NULL;
	  char *encoded_text = NULL;
	  stream_t filter = NULL;
	  stream_t in_stream = NULL;
	  const char *filter_type = NULL;
	  size_t nbytes = 0, size;
	  char *sp = NULL;

	  start_position = fromstr;

	  fromcode = strtok_r (start_position + 2, "?", &sp);
	  encoding_type = strtok_r (NULL, "?", &sp);
	  encoded_text = strtok_r (NULL, "?", &sp);
	  if (sp == NULL || sp[0] != '=')
	    {
	      status = MU_ERR_BAD_2047_INPUT;
	      break;
	    }
      
	  if (fromcode == NULL
	      || encoding_type == NULL
	      || encoded_text == NULL)
	    {
	      status = MU_ERR_BAD_2047_INPUT;
	      break;
	    }

	  size = strlen (encoded_text);

	  switch (toupper (encoding_type[0]))
	    {
	    case 'B':
	      filter_type = "base64";
	      break;
	      
	    case 'Q':
	      filter_type = "Q";
	      break;

	    default:
	      status = MU_ERR_BAD_2047_INPUT;
	      break;
	    }
	  
	  if (status != 0)
	    break;

	  memory_stream_create (&in_stream, 0, 0);
	  stream_write (in_stream, encoded_text, size, 0, NULL);
	  filter_create (&filter, in_stream, filter_type, MU_FILTER_DECODE,
			 MU_STREAM_READ);

	  while (stream_sequential_read (filter, buffer + bufpos,
					 bufsize - bufpos,
					 &nbytes) == 0 && nbytes)
	    {
	      /* FIXME: Need to convert character set */
	      bufpos += nbytes;
	    }

	  stream_close (filter);
	  stream_destroy (&filter, stream_get_owner (filter));
	  
	  fromstr = sp + 1;
	  run_count = 1;
	}
      else if (run_count)
	{
	  if (*fromstr == ' ' || *fromstr == '\t')
	    {
	      run_count++;
	      fromstr++;
	      continue;
	    }
	  else
	    {
	      if (--run_count)
		{
		  memcpy (buffer + bufpos, fromstr - run_count, run_count);
		  bufpos += run_count;
		  run_count = 0;
		}
		
	      buffer[bufpos++] = *fromstr++;
	    }
	}
      else
	buffer[bufpos++] = *fromstr++;
    }
  
  if (*fromstr)
    {
      size_t len = strlen (fromstr);
      if (bufpos + len + 1 > bufsize) /* just in case */
	status = MU_ERR_BAD_2047_INPUT;
      else
	{
	  memcpy (buffer + bufpos, fromstr, strlen (fromstr));
	  bufpos += strlen (fromstr);
	}
    }

  buffer[bufpos++] = 0;
  
  free (tmpcopy);
  *ptostr = realloc (buffer, bufpos);
  return status;
}



/* ==================================================
   RFC 2047 Encoder
   ================================================== */

#define MAX_QUOTE 75

/* Be more conservative in what we quote than in RFC2045, as in some
   circumstances, additional symbols (like parenthesis) must be quoted
   in headers. This is never a problem for the recipient, except for
   the extra overhead in the message size */
static int
must_quote (char c)
{
  if (((c >  32) && (c <= 57))  || 
      ((c >= 64) && (c <= 126)))
    return 0;

  return 1;
}


/* State of the encoder */
typedef struct _encoder rfc2047_encoder;

struct _encoder {
  /* Name of the encoding (either B or Q) */
  char encoding;

  /* Charset of the input stream */
  const char * charset;

  /* Compute the size of the next character (in bytes), according to
     the charset */
  int (* charcount) (const char *);

  /* Size of the next character (in bytes) */
  int charblock;

  /* TRUE if we need to open a quoted-word at the next byte */
  int must_open;

  /* Pointer on the current input byte */
  const unsigned char * src;

  /* Pointer on the current output byte and on the complete output */
  char * dst, * result;

  /* todo: number of bytes remaining in the input, done: number of
     bytes written in the output, quotesize: number of bytes in the
     current quoted-word */
  int todo, done, quotesize;

  /* Virtual methods implemented for the encoders:
      
      count: return how many bytes would be used by inserting the
             current input and updates 'charblock'
      next:  quote the current input byte on the output
      flush: output any pending byte
  */
  int  (* count) (rfc2047_encoder * enc);
  int  (* next)  (rfc2047_encoder * enc);
  void (* flush) (rfc2047_encoder * enc);

  /* Extra data for the Base64 encoder */
  unsigned char buffer [4];
  int  state;
};


/* --------------------------------------------------
   Quoted-words building blocks 
   -------------------------------------------------- */

/* Write the opening of a quoted-word and return the minimum number of
   bytes it will use */
static int
_open_quote (const char * charset,
	    char encoding,
	    char ** dst, int * done)
{
  int len = strlen (charset) + 5;

  (* done) += len;
  
  if (* dst)
    {
      sprintf (* dst, "=?%s?%c?", charset, encoding);
      (* dst) += len;
    }

  /* in the initial length of the quote we already count the final ?= */
  return len + 2;
}

/* Terminate a quoted-word */
static void
_close_quote (char ** dst, int * done)
{
  * done += 2;
  
  if (* dst)
    {
      strcpy (* dst, "?=");
      (* dst) += 2;
    }
}


/* Call this function before the beginning of a quoted-word */
static void
init_quoted (rfc2047_encoder * enc)
{
  enc->must_open = 1;
}

/* Insert the current byte in the quoted-word (handling maximum
   quoted-word sizes,...) */
static void
insert_quoted (rfc2047_encoder * enc)
{
  if (enc->must_open)
    {
      enc->must_open = 0;

      /* The quotesize holds the known size of the quoted-word, even
	 if all the bytes have not yet been inserted in the output
	 stream. */
      enc->quotesize = 
	_open_quote (enc->charset, enc->encoding, 
		     & enc->dst, & enc->done) + enc->count (enc);
    }
  else 
    {
      if (enc->charblock == 0)
	{
	  /* The quotesize holds the known size of the quoted-word,
	     even if all the bytes have not yet been inserted in the
	     output stream. */
	  enc->quotesize += enc->count (enc);
	  if (enc->quotesize > MAX_QUOTE)
	    {
	      /* Start a new quoted-word */
	      _close_quote (& enc->dst, & enc->done);
	      
	      if (enc->dst) * (enc->dst ++) = ' ';
	      enc->done ++;
	      
	      enc->quotesize = _open_quote (enc->charset, enc->encoding, 
					    & enc->dst, & enc->done);
	    }
	}
    }

  /* We are ready to process one more byte from the input stream */
  enc->charblock --;
  enc->next (enc);
}

/* Flush the current quoted-word */
static void
flush_quoted (rfc2047_encoder * enc)
{
  if (enc->must_open) return;

  enc->flush (enc);
  _close_quote (& enc->dst, & enc->done);
}


/* Insert the current byte unquoted */
static void
insert_unquoted (rfc2047_encoder * enc)
{
  if (enc->dst) * (enc->dst ++) = * (enc->src);
  enc->src ++;
  enc->todo --;
  enc->done ++;
}


/* Check if the next word will need to be quoted */
static int
is_next_quoted (const char * src) 
{
  while (isspace (* src)) src ++;

  while (* src) 
    {
      if (isspace (* src)) return 0;
      if (must_quote (* src)) return 1;

      src ++;
    }

  return 0;
}


/* --------------------------------------------------
   Known character encodings
   -------------------------------------------------- */

static int
ce_single_byte (const char * src)
{
  return 1;
}

static int
ce_utf_8 (const char * src)
{
  unsigned char c = * src;

  if (c <= 0x7F) return 1;

  if (c >= 0xFC) return 6;
  if (c >= 0xF8) return 5;
  if (c >= 0xF0) return 4;
  if (c >= 0xE0) return 3;
  if (c >= 0xC0) return 2;

  /* otherwise, this is not a first byte (and the UTF-8 is possibly
     broken), continue with a single byte. */
  return 1;
}


/* --------------------------------------------------
   Quoted-printable encoder 
   -------------------------------------------------- */

static void
qp_init (rfc2047_encoder * enc)
{
  return;
}

static int
qp_count (rfc2047_encoder * enc)
{
  int len = 0, todo;
  unsigned const char * curr;

  /* count the size of a complete (multibyte) character */
  enc->charblock = enc->charcount (enc->src);

  for (todo = 0, curr = enc->src ;
       todo < enc->charblock && * curr; 
       todo ++, curr ++)
    {
      len += must_quote (* curr) ? 3 : 1;
    }

  return len;
}

static const char _hexdigit[16] = "0123456789ABCDEF";

static int
qp_next (rfc2047_encoder * enc)
{
  int done;

  if (* enc->src == '_' || must_quote (* enc->src))
    {
      /* special encoding of space as a '_' to increase readability */
      if (* enc->src == ' ')
	{
	  if (enc->dst)
	    {
	      * (enc->dst ++) = '_';
	    }

	  done = 1;
	}
      else {
	/* default encoding */
	if (enc->dst)
	  {
	    * (enc->dst ++) = '=';
	    * (enc->dst ++) = _hexdigit [* (enc->src) >> 4];
	    * (enc->dst ++) = _hexdigit [* (enc->src) & 0xF];
	  }
	
	done = 3;
      }
    }
  else
    {
      if (enc->dst)
	{
	  * (enc->dst ++) = * enc->src;
	}

      done = 1;
    }

  enc->src ++;

  enc->done += done;
  enc->todo --;

  return done;
}

static void
qp_flush (rfc2047_encoder * enc)
{
  return;
}


/* --------------------------------------------------
   Base64 encoder 
   -------------------------------------------------- */

const char *b64 =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void
base64_init (rfc2047_encoder * enc)
{
  enc->state = 0;
  return;
}

static int
base64_count (rfc2047_encoder * enc)
{
  int len = 0, todo;

  /* Check the size of a complete (multibyte) character */
  enc->charblock = enc->charcount (enc->src);

  for (todo = 0 ; todo < enc->charblock; todo ++)
    {
      /* Count the size of the encoded block only once, at the first
	 byte transmitted. */
      len += ((enc->state + todo) % 3 == 0) ? 4 : 0;
    }

  return len;
}

static int
base64_next (rfc2047_encoder * enc)
{
  enc->buffer [enc->state ++] = * (enc->src ++);
  enc->todo --;

  if (enc->state < 3) return 0;
  
  /* We have a full quantum */
  if (enc->dst)
    {
      * (enc->dst ++) = b64 [(enc->buffer[0] >> 2)];
      * (enc->dst ++) = b64 [((enc->buffer[0] & 0x3) << 4) | (enc->buffer[1] >> 4)];
      * (enc->dst ++) = b64 [((enc->buffer[1] & 0xF) << 2) | (enc->buffer[2] >> 6)];
      * (enc->dst ++) = b64 [(enc->buffer[2] & 0x3F)];
    }
  
  enc->done += 4;
  
  enc->state = 0;
  return 4;
}

static void
base64_flush (rfc2047_encoder * enc)
{
  if (enc->state == 0) return;

  if (enc->dst) 
    {
      switch (enc->state)
	{
	case 1:
	  * (enc->dst ++) = b64 [(enc->buffer[0] >> 2)];
	  * (enc->dst ++) = b64 [((enc->buffer[0] & 0x3) << 4)];
	  * (enc->dst ++) = '=';
	  * (enc->dst ++) = '=';
	  break;
	
	case 2:
	  * (enc->dst ++) = b64 [(enc->buffer[0] >> 2)];
	  * (enc->dst ++) = b64 [((enc->buffer[0] & 0x3) << 4) | (enc->buffer[1] >> 4)];
	  * (enc->dst ++) = b64 [((enc->buffer[1] & 0xF) << 2)];
	  * (enc->dst ++) = '=';
	  break;
	}
    }

  enc->done += 4;
  enc->state = 0;
  return;
}


/* States of the RFC2047 encoder */
enum {
  ST_SPACE,   /* waiting for non-quoted whitespace */
  ST_WORD,    /* waiting for non-quoted word */
  ST_QUOTED,  /* waiting for quoted word */
  ST_QUOTED_SPACE, /* waiting for quoted whitespace */
};

/**
   Encode a header according to RFC 2047
   
   @param charset
     Charset of the text to encode
   @param encoding
     Requested encoding (must be "base64" or "quoted-printable")
   @param text
     Actual text to encode
   @param result [OUT]
     Encoded string

   @return 0 on success
*/
int
rfc2047_encode (const char *charset, const char *encoding,
		const char *text, char ** result)
{
  rfc2047_encoder enc;

  int is_compose;
  int state;

  if (! charset  ||
      ! encoding || 
      ! text     || 
      ! result) return EINVAL;

  /* Check for a known encoding */
  do 
    {
      if (strcasecmp (encoding, "base64") == 0) 
	{
	  base64_init (& enc);
	  enc.encoding = 'B';
	  enc.next  = base64_next;
	  enc.count = base64_count;
	  enc.flush = base64_flush;
	  break;
	}
      
      if (strcasecmp (encoding, "quoted-printable") == 0) 
	{
	  qp_init (& enc);
	  enc.encoding = 'Q';
	  enc.next  = qp_next;
	  enc.count = qp_count;
	  enc.flush = qp_flush;
	  break;
	}
      
      return ENOENT;
    } 
  while (0);

  /* Check for a known charset */
  do
    {
      if (strcasecmp (charset, "utf-8") == 0)
	{
	  enc.charcount = ce_utf_8;
	  break;
	}
      
      enc.charcount = ce_single_byte;
    }
  while (0);

  enc.dst = NULL;
  enc.charset = charset;

  /* proceed in two passes: estimate the required space, then fill */
  for (is_compose = 0 ; is_compose <= 1 ; is_compose ++) 
    {
      state = ST_SPACE;

      enc.src  = text;
      enc.todo = strlen (text);
      enc.done = 0;

      while (enc.todo) 
	{
	  
	  switch (state) 
	    {
	    case ST_SPACE:
	      if (isspace (* enc.src)) 
		{
		  insert_unquoted (& enc);
		  break;
		}

	      if (is_next_quoted (enc.src)) 
		{
		  init_quoted (& enc);
		  state = ST_QUOTED;
		}
	      else 
		{
		  state = ST_WORD;
		}
	      break;

	    case ST_WORD:
	      if (isspace (* enc.src)) 
		{
		  state = ST_SPACE;
		  break;
		}

	      insert_unquoted (& enc);
	      break;
	      
	    case ST_QUOTED:
	      if (isspace (* enc.src))
		{
		  if (is_next_quoted (enc.src))
		    {
		      state = ST_QUOTED_SPACE;
		    }
		  else
		    {
		      flush_quoted (& enc);
		      state = ST_SPACE;
		    }
		  break;
		}

	      insert_quoted (& enc);
	      break;
	      
	    case ST_QUOTED_SPACE:
	      if (! isspace (* enc.src))
		{
		  state = ST_QUOTED;
		  break;
		}

	      insert_quoted (& enc);
	      break;
	    }
	}

      if (state == ST_QUOTED ||
	  state == ST_QUOTED_SPACE)
	{
	  flush_quoted (& enc);
	}

      if (enc.dst == NULL) 
	{
	  enc.dst = malloc (enc.done + 1);
	  if (enc.dst == NULL) return -ENOMEM;
	  enc.result = enc.dst;
	}
    }

  * (enc.dst) = '\0';
  * result = enc.result;

  return 0;
}
