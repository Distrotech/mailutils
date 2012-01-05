/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003-2007, 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <mailutils/stream.h>
#include <mailutils/filter.h>
#include <mailutils/errno.h>
#include <mailutils/mime.h>
#include <mailutils/util.h>

int
getword (char **pret, const char **pstr, int delim)
{
  size_t len;
  char *ret;
  const char *start = *pstr;
  const char *end = strchr (start, delim);

  free (*pret);
  *pret = NULL;
  if (!end)
    return MU_ERR_BAD_2047_INPUT;
  len = end - start;
  ret = malloc (len + 1);
  if (!ret)
    return ENOMEM;
  memcpy (ret, start, len);
  ret[len] = 0;
  *pstr = end + 1;
  *pret = ret;
  return 0;
}
    
static int
_rfc2047_decode_param (const char *tocode, const char *input,
		       struct mu_mime_param *param)
{
  int status = 0;
  const char *fromstr;
  size_t run_count = 0;
  char *fromcode = NULL;
  char *encoding_type = NULL;
  char *encoded_text = NULL;
  char *tocodetmp = NULL;
  mu_stream_t str;

  memset (param, 0, sizeof (*param));

  status = mu_memory_stream_create (&str, MU_STREAM_RDWR);
  if (status)
    return status;

  if (tocode && (param->cset = strdup (tocode)) == NULL)
    {
      mu_stream_destroy (&str);
      return ENOMEM;
    }
  
  fromstr = input;
  
  while (*fromstr)
    {
      if (strncmp (fromstr, "=?", 2) == 0)
	{
	  mu_stream_t filter = NULL;
	  mu_stream_t in_stream = NULL;
	  const char *filter_type = NULL;
	  size_t size;
	  const char *sp = fromstr + 2;
	  char *lang;
	  
	  status = getword (&fromcode, &sp, '?');
	  if (status)
	    break;
	  lang = strchr (fromcode, '*');
	  if (lang)
	    *lang++ = 0;
	  if (!param->cset)
	    {
	      param->cset = strdup (fromcode);
	      if (!param->cset)
		{
		  status = ENOMEM;
		  break;
		}
	    }
	  if (lang && !param->lang && (param->lang = strdup (lang)) == NULL)
	    {
	      status = ENOMEM;
	      break;
	    }
	  if (!tocode)
	    {
	      if ((tocodetmp = strdup (fromcode)) == NULL)
		{
		  status = ENOMEM;
		  break;
		}
	      tocode = tocodetmp;
	    }
	  status = getword (&encoding_type, &sp, '?');
	  if (status)
	    break;
	  status = getword (&encoded_text, &sp, '?');
	  if (status)
	    break;
	  if (sp == NULL || sp[0] != '=')
	    {
	      status = MU_ERR_BAD_2047_INPUT;
	      break;
	    }
      
	  size = strlen (encoded_text);

	  switch (encoding_type[0])
	    {
            case 'b':
	    case 'B':
	      filter_type = "base64";
	      break;
	     
            case 'q': 
	    case 'Q':
	      filter_type = "Q";
	      break;

	    default:
	      status = MU_ERR_BAD_2047_INPUT;
	      break;
	    }
	  
	  if (status != 0)
	    break;

	  mu_static_memory_stream_create (&in_stream, encoded_text, size);
	  mu_stream_seek (in_stream, 0, MU_SEEK_SET, NULL);
	  status = mu_decode_filter (&filter, in_stream, filter_type,
				     fromcode, tocode);
	  mu_stream_unref (in_stream);
	  if (status != 0)
	    break;
	  status = mu_stream_copy (str, filter, 0, NULL);
	  mu_stream_destroy (&filter);

	  if (status)
	    break;
	  
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
		  status = mu_stream_write (str, fromstr - run_count,
					    run_count, NULL);
		  if (status)
		    break;
		  run_count = 0;
		}
	      status = mu_stream_write (str, fromstr, 1, NULL);
	      if (status)
		break;
	      fromstr++;
	    }
	}
      else
	{
	  status = mu_stream_write (str, fromstr, 1, NULL);
	  if (status)
	    break;
	  fromstr++;
	}
    }
  
  if (status == 0 && *fromstr)
    status = mu_stream_write (str, fromstr, strlen (fromstr), NULL);

  free (fromcode);
  free (encoding_type);
  free (encoded_text);
  free (tocodetmp);
  
  if (status == 0)
    {
      mu_off_t size;

      mu_stream_size (str, &size);
      param->value = malloc (size + 1);
      if (!param->value)
	status = ENOMEM;
      else
	{
	  mu_stream_seek (str, 0, MU_SEEK_SET, NULL);
	  status = mu_stream_read (str, param->value, size, NULL);
	  param->value[size] = 0;
	}
    }
  
  mu_stream_destroy (&str);
  return status;
}

int
mu_rfc2047_decode_param (const char *tocode, const char *input,
			 struct mu_mime_param *param)
{
  int rc;
  struct mu_mime_param tmp;

  if (!input)
    return EINVAL;
  if (!param)
    return MU_ERR_OUT_PTR_NULL;
  rc = _rfc2047_decode_param (tocode, input, &tmp);
  if (rc == 0)
    *param = tmp;
  return rc;
}

int
mu_rfc2047_decode (const char *tocode, const char *input, char **ptostr)
{
  int rc;
  struct mu_mime_param param;
  
  if (!input)
    return EINVAL;
  if (!ptostr)
    return MU_ERR_OUT_PTR_NULL;
  rc = _rfc2047_decode_param (tocode, input, &param);
  free (param.cset);
  free (param.lang);
  if (rc == 0)
    *ptostr = param.value;
  return rc;
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
mu_rfc2047_encode (const char *charset, const char *encoding,
		   const char *text, char **result)
{
  mu_stream_t input_stream;
  mu_stream_t output_stream;
  int rc;
  
  if (charset == NULL || encoding == NULL || text == NULL)
    return EINVAL;

  if (strcmp (encoding, "base64") == 0)
    encoding = "B";
  else if (strcmp (encoding, "quoted-printable") == 0)
    encoding = "Q";
  else if (encoding[1] || !strchr ("BQ", encoding[0]))
    return MU_ERR_BAD_2047_ENCODING;

  rc = mu_static_memory_stream_create (&input_stream, text, strlen (text));
  if (rc)
    return rc;
  rc = mu_filter_create (&output_stream, input_stream,
			 encoding, MU_FILTER_ENCODE, MU_STREAM_READ);
  mu_stream_unref (input_stream);
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
	  
	  rc = mu_stream_read (output_stream,
			       p,
			       strlen (text) * 3, &s);

	  strcpy (p + s, "?=");
	}
      else
	rc = ENOMEM;
      mu_stream_destroy (&output_stream);
    }
  else
    mu_stream_destroy (&input_stream);

  return rc;
}
