/* Join strings from an array into a single string.
   Copyright (C) 1999-2001, 2003-2006, 2010-2012 Free Software
   Foundation, Inc.

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
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/argcv.h>
#include <mailutils/errno.h>
#include <mailutils/wordsplit.h>

int
mu_argcv_join (int argc, char **argv, char *delim, enum mu_argcv_escape esc,
	       char **pstring)
{
  size_t i, j, len;
  char *buffer;
  size_t delimlen = strlen (delim);
  int quote_hex = 0;
  
  if (pstring == NULL)
    return EINVAL;

  buffer = malloc (1);
  if (buffer == NULL)
    return ENOMEM;
  *buffer = '\0';

  for (len = i = j = 0; i < argc; i++)
    {
      int quote;
      int toklen;

      switch (esc)
	{
	case mu_argcv_escape_no:
	  toklen = strlen (argv[i]);
	  quote = 0;
	  break;

	case mu_argcv_escape_c:
	  toklen = mu_wordsplit_c_quoted_length (argv[i], quote_hex, &quote);
	  break;

#if 0
	case mu_argcv_escape_sh:
	  FIXME
	  toklen = mu_wordsplit_sh_quoted_length (argv[i], &quote);
	  break;
#endif
	default:
	  return EINVAL;
	}
	
      len += toklen + delimlen;
      if (quote)
	len += 2;
      
      buffer = realloc (buffer, len + 1);
      if (buffer == NULL)
        return ENOMEM;

      if (i != 0)
	{
	  memcpy (buffer + j, delim, delimlen);
	  j += delimlen;
	}
      if (quote)
	buffer[j++] = '"';
      
      switch (esc)
	{
	case mu_argcv_escape_no:
	  memcpy (buffer + j, argv[i], toklen);
	  break;

	case mu_argcv_escape_c:
	  mu_wordsplit_c_quote_copy (buffer + j, argv[i], quote_hex);
	  break;
#if 0
	case mu_argcv_escape_sh:
	  mu_wordsplit_sh_quote_copy (buffer + j, argv[i]);
#endif
	}

      j += toklen;
      if (quote)
	buffer[j++] = '"';
    }

  buffer[j] = 0;
  *pstring = buffer;
  return 0;
}

int
mu_argcv_string (int argc, char **argv, char **pstring)
{
  return mu_argcv_join (argc, argv, " ", mu_argcv_escape_c, pstring);
}
