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
      char *pbuffer = NULL;
      const char *filter_type = NULL;
      size_t nbytes = 0, size;
      char *sp = NULL;
      char *end_position = NULL;

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

