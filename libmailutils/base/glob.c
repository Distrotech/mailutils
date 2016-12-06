/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2009-2012, 2014-2016 Free Software Foundation,
   Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <mailutils/opool.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/glob.h>
#include <regex.h>
#include <string.h>
#include <stdlib.h>

static void
parse_character_class (unsigned char const *str, mu_opool_t pool,
		       unsigned char const **endp)
{
  unsigned char const *cur;

  cur = str + 1;
  if (*cur == '!')
    cur++;
  if (*cur == ']')
    cur++;

  while (*cur && *cur != ']')
    {
      int c = *cur++;
      if (c == '\\')
	cur++;
      else if (c >= 0xc2)
	{
	  size_t len;
	  
	  if (c < 0xe0)
	    len = 1;
	  else if (c < 0xf0)
	    len = 2;
	  else if (c < 0xf8)
	    len = 3;
	  else
	    /* Invalid UTF-8 sequence; skip. */
	    continue;

	  while (len-- && *cur)
	    cur++;
	}
    }

  if (*cur == ']')
    {
      /* Valid character class */
      mu_opool_append_char (pool, *str);
      str++;
      if (*str == '!')
	{
	  mu_opool_append_char (pool, '^');
	  str++;
	}
      while (str < cur)
	{
	  if (*str == '[')
	    mu_opool_append_char (pool, '\\');
	  
	  mu_opool_append_char (pool, *str);
	  str++;
	}
      mu_opool_append_char (pool, ']');
      *endp = cur + 1;
    }
  else
    {
      mu_opool_append_char (pool, '\\');
      mu_opool_append_char (pool, *str);
      str++;
      *endp = str;
    }
}

int
mu_glob_to_regex_opool (char const *pattern, mu_opool_t pool, int flags)
{
  unsigned char const *str = (unsigned char const *) pattern;
  mu_nonlocal_jmp_t jmp;
  
  if (!(flags & MU_GLOBF_SUB))
    flags |= MU_GLOBF_COLLAPSE;

  mu_opool_setup_nonlocal_jump (pool, jmp);
  
  while (*str)
    {
      int c = *str++;
      
      if (c < 0x80)
	{
	  switch (c)
	    {
	    case '\\':
	      mu_opool_append_char (pool, '\\');
	      if (*str && strchr ("?*[", *str))
		{
		  mu_opool_append_char (pool, *str);
		  str++;
		}
	      else
		mu_opool_append_char (pool, '\\');
	      break;

	    case '?':
	      if (flags & MU_GLOBF_SUB)
		mu_opool_append_char (pool, '(');
	      mu_opool_append_char (pool, '.');
	      if (flags & MU_GLOBF_SUB)
		mu_opool_append_char (pool, ')');
	      break;

	    case '*':
	      if (flags & MU_GLOBF_COLLAPSE)
		{
		  while (*str == '*')
		    str++;
		}
	      
	      if (flags & MU_GLOBF_SUB)
		{
		  while (*str == '*')
		    {
		      mu_opool_append (pool, "()", 2);
		      str++;
		    }
		  mu_opool_append_char (pool, '(');
		  mu_opool_append (pool, ".*", 2);
		  mu_opool_append_char (pool, ')');
		}
	      else
		mu_opool_append (pool, ".*", 2);
	      break;

	    case '[':
	      parse_character_class (str - 1, pool, &str);
	      break;
	      
	    case '(':
	    case ')':
	    case '{':
	    case '}':
	    case '^':
	    case '$':
	    case ']':
	    case '|':
	    case '.':
	      mu_opool_append_char (pool, '\\');
	      mu_opool_append_char (pool, c);
	      break;
	      
	    default:
	      mu_opool_append_char (pool, c);
	    }
	}
      else
	{
	  mu_opool_append_char (pool, c);
	  if (c >= 0xc2)
	    {
	      size_t len;
	  
	      if (c < 0xe0)
		len = 1;
	      else if (c < 0xf0)
		len = 2;
	      else if (c < 0xf8)
		len = 3;
	      else
		/* Invalid UTF-8 sequence; skip. */
		continue;

	      for (; len-- && *str; str++)
		mu_opool_append_char (pool, *str);
	    }
	}
    }
  mu_opool_clrjmp (pool);
  return 0;
}

int
mu_glob_to_regex (char **rxstr, char const *pattern, int flags)
{
  mu_opool_t pool;
  int rc;
  mu_nonlocal_jmp_t jmp;
    
  rc = mu_opool_create (&pool, MU_OPOOL_DEFAULT);
  if (rc)
    return rc;
  mu_opool_setup_nonlocal_jump (pool, jmp);
  mu_opool_append_char (pool, '^');
  rc = mu_glob_to_regex_opool (pattern, pool, flags);
  if (rc == 0)
    {
      mu_opool_append_char (pool, '$');
      mu_opool_append_char (pool, 0);
      *rxstr = mu_opool_detach (pool, NULL);
    }
  mu_opool_clrjmp (pool);
  mu_opool_destroy (&pool);
  return rc;
}

int
mu_glob_compile (regex_t *rx, char const *pattern, int flags)
{
  char *str;
  int rc;
  int rxflags;
  
  rc = mu_glob_to_regex (&str, pattern, flags);
  if (rc)
    return rc;

  rxflags = REG_EXTENDED;
  if (flags & MU_GLOBF_ICASE)
    rxflags |= REG_ICASE;
  if (!(flags & MU_GLOBF_SUB))
    rxflags |= REG_NOSUB;
  
  rc = regcomp (rx, str, rxflags);
  if (rc)
    {
      size_t size = regerror (rc, rx, NULL, 0);
      char *errbuf = malloc (size + 1);
      if (errbuf)
	{
	  regerror (rc, rx, errbuf, size);
	  mu_error ("INTERNAL ERROR: can't compile regular expression \"%s\": %s",
		    str, mu_strerror (rc));
	}
      else
	mu_error ("INTERNAL ERROR: can't compile regular expression \"%s\"",
		  str);
      mu_error ("INTERNAL ERROR: expression compiled from globbing pattern: %s",
		pattern);
      free (errbuf);
    }
  free (str);
  
  return rc;
}
