/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2004, 2005 Free Software Foundation, Inc.

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
#include <mailutils/mutil.h>

static int
realloc_buffer (char **bufp, size_t *bufsizep, size_t incr)
{
  size_t newsize = *bufsizep + incr;
  char *newp = realloc (*bufp, newsize);
  if (newp == NULL)
    return 1;
  *bufp = newp;
  *bufsizep = newsize;
  return 0;
}

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

#define BUFINC 128  
#define CHKBUF(count) do {                       \
  if (bufpos+count >= bufsize)                   \
    {                                            \
      size_t s = bufpos + count - bufsize;       \
      if (s < BUFINC)                            \
        s = BUFINC;                              \
      if (realloc_buffer (&buffer, &bufsize, s)) \
	{                                        \
	  free (tmpcopy);                        \
	  free (buffer);                         \
	  return ENOMEM;                         \
	}                                        \
     }                                           \
 } while (0) 
  
  if (!tocode || !input)
    return EINVAL;
  if (!ptostr)
    return MU_ERR_OUT_PTR_NULL;

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
	  status = mu_decode_filter (&filter, in_stream, filter_type, fromcode,
				     tocode);
	  if (status != 0)
	    break;

	  while (stream_sequential_read (filter, buffer + bufpos,
					 bufsize - bufpos,
					 &nbytes) == 0
		 && nbytes)
	    bufpos += nbytes;

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
		  CHKBUF(run_count);
		  memcpy (buffer + bufpos, fromstr - run_count, run_count);
		  bufpos += run_count;
		  run_count = 0;
		}
	      CHKBUF(1);
	      buffer[bufpos++] = *fromstr++;
	    }
	}
      else
	{
	  CHKBUF(1);
	  buffer[bufpos++] = *fromstr++;
	}
    }
  
  if (*fromstr)
    {
      size_t len = strlen (fromstr);
      CHKBUF(len);
      memcpy (buffer + bufpos, fromstr, strlen (fromstr));
      bufpos += strlen (fromstr);
    }

  CHKBUF(1);
  buffer[bufpos++] = 0;
  
  free (tmpcopy);
  *ptostr = realloc (buffer, bufpos);
  return status;
}


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
		const char *text, char **result)
{
  stream_t input_stream;
  stream_t output_stream;
  int rc;
  
  if (charset == NULL || encoding == NULL || text == NULL)
    return MU_ERR_BAD_2047_INPUT;

  if (strcmp (encoding, "base64") == 0)
    encoding = "B";
  else if (strcmp (encoding, "quoted-printable") == 0)
    encoding = "Q";
  else if (encoding[1] || !strchr ("BQ", encoding[0]))
    return MU_ERR_BAD_2047_ENCODING;

  rc = memory_stream_create (&input_stream, 0, 0);
  if (rc)
    return rc;
  
  stream_sequential_write (input_stream, text, strlen (text));

  rc = mu_filter_create (&output_stream, input_stream,
		      encoding, MU_FILTER_ENCODE, MU_STREAM_READ);
  if (rc == 0)
    {
      /* Assume strlen(qp_encoded_text) <= strlen(text) * 3 */
      /* malloced length is composed of:
	 "=?"  
	 charset 
	 "?"
	 B or Q
	 "?" 
	 encoded_text
	 "?="
	 zero terminator */
      
      *result = malloc (2 + strlen (charset) + 3 + strlen (text) * 3 + 3);
      if (*result)
	{
	  char *p = *result;
	  size_t s;
	  
	  p += sprintf (p, "=?%s?%s?", charset, encoding);
	  
	  rc = stream_sequential_read (output_stream,
				       p,
				       strlen (text) * 3, &s);

	  strcpy (p + s, "?=");
	}
      else
	rc = ENOMEM;
      stream_destroy (&output_stream, NULL);
    }
  else
    stream_destroy (&input_stream, NULL);

  return rc;
}
