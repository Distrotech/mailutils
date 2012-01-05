/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2009-2012 Free Software Foundation, Inc.

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
#include <string.h>
#include <errno.h>
#include <mailutils/nls.h>

static int
_parse_lc_all (const char *arg, struct mu_lc_all *str, int flags)
{
  char *s;
  size_t n;
  
  n = strcspn (arg, "_.@");
  if (flags & MU_LC_LANG)
    {
      s = malloc (n + 1);
      if (!s)
	return ENOMEM;
      memcpy (s, arg, n);
      s[n] = 0;
      str->language = s;
      str->flags |= MU_LC_LANG;
    }
  else
    str->language = NULL;
  arg += n;

  if (arg[0] == '_')
    {
      arg++;
  
      n = strcspn (arg, ".@");
      if (flags & MU_LC_TERR)
	{
	  s = malloc (n + 1);
	  if (!s)
	    return ENOMEM;
	  memcpy (s, arg, n);
	  s[n] = 0;
	  str->territory = s;
	  str->flags |= MU_LC_TERR;
	}
      else
	str->territory = NULL;
      arg += n;
    }

  if (arg[0] == '.')
    {
      arg++;

      n = strcspn (arg, "@");
      if (flags & MU_LC_CSET)
	{
	  s = malloc (n + 1);
	  if (!s)
	    return ENOMEM;
	  memcpy (s, arg, n);
	  s[n] = 0;
	  str->charset = s;
	  str->flags |= MU_LC_CSET;
	}
      else
	str->charset = NULL;
      arg += n;
    }

  if (arg[0])
    {
      arg++;
      if (flags & MU_LC_MOD)
	{
	  str->modifier = strdup (arg);
	  if (!str->modifier)
	    return ENOMEM;
	  str->flags |= MU_LC_MOD;
	}
    }
  return 0;
}

void
mu_lc_all_free (struct mu_lc_all *str)
{
  free (str->language);
  free (str->territory);
  free (str->charset);
  free (str->modifier);
}

int
mu_parse_lc_all (const char *arg, struct mu_lc_all *str, int flags)
{
  int rc;

  memset (str, 0, sizeof (str[0]));
  if (!arg)
    {
      if (flags & MU_LC_LANG)
	{
	  str->language = strdup ("C");
	  if (!str->language)
	    return ENOMEM;
	}
      return 0;
    }
  
  rc = _parse_lc_all (arg, str, flags);
  if (rc == 0 && !str->charset)
    {
      const char *charset = mu_charset_lookup (str->language, str->territory);
      if (charset)
	{
	  str->charset = strdup (charset);
	  if (!str->charset)
	    rc = ENOMEM;
	}
    }
  if (rc)
    mu_lc_all_free (str);
  return rc;
}
