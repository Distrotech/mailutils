/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

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
      char *fromcode = NULL;
      char *encoding_type = NULL;
      char *encoded_text = NULL;
      stream_t filter = NULL;
      stream_t in_stream = NULL;
      const char *filter_type = NULL;
      size_t nbytes = 0, size;
      char *sp = NULL;

      start_position = strstr (fromstr, "=?");
      if (!start_position)
	break;

      /* Copy the unencoded part */
      nbytes = start_position - fromstr;
      if (bufpos + nbytes > bufsize) /* just in case */
	{
	  status = MU_ERR_BAD_2047_INPUT;
	  break;
	}
	  
      memcpy (buffer + bufpos, fromstr, nbytes);
      bufpos += nbytes;
      
      fromcode = strtok_r (start_position + 2, "?", &sp);
      encoding_type = strtok_r (NULL, "?", &sp);
      encoded_text = strtok_r (NULL, "?", &sp);
      if (sp[0] != '=')
	{
	  status = MU_ERR_BAD_2047_INPUT;
	  break;
	}
      
      if (fromcode == NULL || encoding_type == NULL || encoded_text == NULL)
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
	  filter_type = "quoted-printable";
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

      while (stream_sequential_read (filter, buffer + bufpos, bufsize - bufpos,
				     &nbytes) == 0 && nbytes)
        {
          /* FIXME: Need to convert character set */
	  bufpos += nbytes;
        }

      stream_close (filter);
      stream_destroy (&filter, stream_get_owner (filter));
      
      fromstr = sp + 1;
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



/* --------------------------------------------------
   RFC 2047 Encoder
   -------------------------------------------------- */

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

  /* Charset of the encoded data */
  const char * charset;

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
      current input
      next:  quote the current input byte on the output
      flush: output any pending byte and close the quoted-word
  */
  int  (* count) (rfc2047_encoder * enc);
  void (* next)  (rfc2047_encoder * enc);
  void (* flush) (rfc2047_encoder * enc);

  /* Extra data for the Base64 encoder */
  unsigned char buffer [4];
  int  state;
};


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
  int size;

  if (enc->must_open)
    {
      enc->must_open = 0;
      enc->quotesize = _open_quote (enc->charset, enc->encoding, 
				    & enc->dst, & enc->done);
    }
  else 
    {
      size = enc->count (enc);
  
      if (enc->quotesize + size > MAX_QUOTE)
	{
	  _close_quote (& enc->dst, & enc->done);

	  if (enc->dst) * (enc->dst ++) = ' ';
	  enc->done ++;

	  enc->quotesize = _open_quote (enc->charset, enc->encoding, 
					& enc->dst, & enc->done);
	}
    }
  
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
  return must_quote (* enc->src) ? 3 : 1;
}

static const char _hexdigit[16] = "0123456789ABCDEF";

static void
qp_next (rfc2047_encoder * enc)
{
  if (* enc->src == '_' || must_quote (* enc->src))
    {
      /* special encoding of space as a '_' to increase readability */
      if (* enc->src == ' ')
	{
	  if (enc->dst)
	    {
	      * (enc->dst ++) = '_';
	      enc->src ++;
	    }
	
	enc->done ++;
	enc->quotesize ++;
	}
      else {
	/* default encoding */
	if (enc->dst)
	  {
	    * (enc->dst ++) = '=';
	    * (enc->dst ++) = _hexdigit [* (enc->src) >> 4];
	    * (enc->dst ++) = _hexdigit [* (enc->src) & 0xF];
	    
	    enc->src ++;
	  }
	
	enc->done += 3;
	enc->quotesize += 3;
      }
    }
  else
    {
      if (enc->dst)
	{
	  * (enc->dst ++) = * (enc->src ++);
	}

      enc->done ++;
      enc->quotesize ++;
    }

  enc->todo --;
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
  /* Count the size of the encoded block only once, at the first byte
     transmitted. */
  if (enc->state == 0) return 4;
  return 0;
}

static void
base64_next (rfc2047_encoder * enc)
{
  enc->buffer [enc->state ++] = * (enc->src ++);

  enc->todo --;

  /* We have a full quantum */
  if (enc->state >= 3) 
    {
      if (enc->dst)
	{
	  * (enc->dst ++) = b64 [(enc->buffer[0] >> 2)];
	  * (enc->dst ++) = b64 [((enc->buffer[0] & 0x3) << 4) | (enc->buffer[1] >> 4)];
	  * (enc->dst ++) = b64 [((enc->buffer[1] & 0xF) << 2) | (enc->buffer[2] >> 6)];
	  * (enc->dst ++) = b64 [(enc->buffer[2] & 0x3F)];
	}

      enc->done += 4;
      enc->quotesize += 4;

      enc->state = 0;
    }
  return;
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
  enc->quotesize += 4;
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

  do 
    {
      if (strcmp (encoding, "base64") == 0) 
	{
	  base64_init (& enc);
	  enc.encoding = 'B';
	  enc.next  = base64_next;
	  enc.count = base64_count;
	  enc.flush = base64_flush;
	  break;
	}
      
      if (strcmp (encoding, "quoted-printable") == 0) 
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

  enc.dst = NULL;
  enc.charset = charset;

  /* proceed in two passes: count, then fill */
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
