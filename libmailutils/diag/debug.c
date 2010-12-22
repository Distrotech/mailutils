/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007, 2008, 2010 Free
   Software Foundation, Inc.

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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/nls.h>
#include <mailutils/cctype.h>
#include <mailutils/wordsplit.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

int mu_debug_line_info;          /* Debug messages include source locations */

struct debug_category
{
  char *name;
  mu_debug_level_t level;
  int isset;
};

static struct debug_category default_cattab[] = {
  { "all" },
#define __MU_DEBCAT_C_ARRAY
#include <mailutils/sys/debcat.h>
#undef __MU_DEBCAT_C_ARRAY
};

static struct debug_category *cattab = default_cattab;
static size_t catcnt = MU_ARRAY_SIZE (default_cattab);
static size_t catmax = 0;
#define MU_DEFAULT_CATMAX 256;

size_t
mu_debug_register_category (char *name)
{
  struct debug_category *newtab;
  size_t n;

  if (cattab == default_cattab)
    {
      /* Reallocate the array */
      n = 2 * catcnt;

      newtab = calloc (n, sizeof (newtab[0]));
      if (!newtab)
	{
	  mu_error (_("cannot reallocate debug category table"));
	  return (size_t)-1;
	}
      memcpy (newtab, cattab, catcnt * sizeof (cattab[0]));
      cattab = newtab;
      catmax = n;
    }
  else if (catcnt == catmax)
    {
      n = catmax + MU_DEFAULT_CATMAX;
      newtab = realloc (cattab, n * sizeof (cattab[0]));
      if (!newtab)
	{
	  mu_error (_("cannot reallocate debug category table"));
	  return (size_t)-1;
	}
      else
	{
	  cattab = newtab;
	  catmax = n;
	}
    }
  cattab[catcnt].name = name;
  cattab[catcnt].level = 0;
  cattab[catcnt].isset = 0;
  return catcnt++;
}

size_t
mu_debug_next_handle ()
{
  return catcnt ? catcnt : 1;
}

int
mu_debug_level_p (mu_debug_handle_t catn, mu_debug_level_t level)
{
  return catn < catcnt
    && ((cattab[catn].isset ? cattab[catn].level : cattab[0].level) &
	MU_DEBUG_LEVEL_MASK (level));
}

int
mu_debug_category_match (mu_debug_handle_t catn, mu_debug_level_t mask)
{
  return catn < catcnt
    && ((cattab[catn].isset ? cattab[catn].level : cattab[0].level) & mask);
}

static mu_debug_handle_t
find_category (const char *name, size_t len)
{
  mu_debug_handle_t i;

  for (i = 0; i < catcnt; i++)
    {
      if (strlen (cattab[i].name) == len &&
	  memcmp (cattab[i].name, name, len) == 0)
	return i;
    }
  return (mu_debug_handle_t)-1;
}

void
mu_debug_enable_category (const char *catname, size_t catlen,
			  mu_debug_level_t level)
{
  mu_debug_handle_t catn = find_category (catname, catlen);
  if (catn == (mu_debug_handle_t)-1)
    {
      mu_error (_("unknown category: %.*s"), (int) catlen, catname);
      return;
    }
  cattab[catn].level = level;
  cattab[catn].isset = 1;
}

void
mu_debug_disable_category (const char *catname, size_t catlen)
{
  size_t catn = find_category (catname, catlen);
  if (catn == (mu_debug_handle_t)-1)
    {
      mu_error (_("unknown category: %.*s"), (int) catlen, catname);
      return;
    }
  cattab[catn].isset = 0;
}

int
mu_debug_category_level (const char *catname, size_t catlen,
			 mu_debug_level_t *plev)
{
  mu_debug_handle_t catn;

  if (catname)
    {
      catn = find_category (catname, catlen);
      if (catn == (mu_debug_handle_t)-1) 
	return MU_ERR_NOENT;
    }
  else
    catn = 0;
  *plev = cattab[catn].level;
  return 0;
}

void
mu_debug_set_category_level (mu_debug_handle_t catn, mu_debug_level_t level)
{
  if (catn < catcnt)
    {
      cattab[catn].isset = 1;
      cattab[catn].level = level;
    }
  else
    abort ();
}
  
static void
parse_spec (const char *spec)
{
  char *q;
  mu_debug_level_t level;

  if (mu_isdigit (*spec))
    {
      level = strtoul (spec, &q, 0);
      if (*q)
	mu_error (_("%s: wrong debug spec near %s"), spec, q);
      else
	{
	  cattab[0].level = level;
	  cattab[0].isset = 1;
	}
      return;
    }

  if (*spec == '!')
    mu_debug_disable_category (spec + 1, strlen (spec + 1));
  else
    {
      char *p;
      size_t len;
      
      p = strchr (spec, '.');
      if (p)
	{
	  struct mu_wordsplit ws;

	  len = p - spec;
	  ws.ws_delim = ",";
	  if (mu_wordsplit (p + 1, &ws,
			    MU_WRDSF_DELIM|MU_WRDSF_WS|
			    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
	    {
	      mu_error (_("cannot split line `%s': %s"), p + 1,
			mu_wordsplit_strerror (&ws));
	      return;
	    }
	  else
	    {
	      size_t i;
	      unsigned lev = 0;
	      unsigned xlev = 0;
	      
	      for (i = 0; i < ws.ws_wordc; i++)
		{
		  char *s = ws.ws_wordv[i];
		  int exact = 0;
		  unsigned n;
		  unsigned *tgt = &lev;
		  
		  if (*s == '!')
		    {
		      tgt = &xlev;
		      s++;
		    }
		  if (*s == '=')
		    {
		      exact = 1;
		      s++;
		    }

		  if (strcmp (s, "error") == 0)
		    n = MU_DEBUG_ERROR;
		  else if (strcmp (s, "prot") == 0)
		    n = MU_DEBUG_PROT;
		  else if (strlen (s) == 6 && memcmp (s, "trace", 5) == 0 &&
			   mu_isdigit (s[5]))
		    n = MU_DEBUG_TRACE0 + s[5] - '0';
		  else
		    {
		      mu_error (_("unknown level `%s'"), s);
		      continue;
		    }

		  if (exact)
		    *tgt |= MU_DEBUG_LEVEL_MASK (n);
		  else
		    *tgt |= MU_DEBUG_LEVEL_UPTO (n);
		}

	      level = lev & ~xlev;
	    }
	}
      else
	{
	  len = strlen (spec);
	  level = MU_DEBUG_LEVEL_UPTO (MU_DEBUG_PROT);
	}
      mu_debug_enable_category (spec, len, level);
    }
}

void
mu_debug_parse_spec (const char *spec)
{
  struct mu_wordsplit ws;

  ws.ws_delim = ";";
  if (mu_wordsplit (spec, &ws,
		    MU_WRDSF_DELIM|MU_WRDSF_WS|
		    MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
    {
      mu_error (_("cannot split line `%s': %s"), spec,
		mu_wordsplit_strerror (&ws));
    }
  else
    {
      size_t i;
    
      for (i = 0; i < ws.ws_wordc; i++)
	parse_spec (ws.ws_wordv[i]);
      mu_wordsplit_free (&ws);
    }
}

void
mu_debug_clear_all ()
{
  int i;

  for (i = 0; i < catcnt; i++)
    cattab[i].isset = 0;
}

#define _LEVEL_ALL MU_DEBUG_LEVEL_UPTO(MU_DEBUG_PROT)

static char *mu_debug_level_str[] = {
  "error",
  "trace0",
  "trace1",
  "trace2",
  "trace3",
  "trace4",
  "trace5",
  "trace6",
  "trace7",
  "trace8",
  "trace9",
  "prot"   
};

static int
name_matches (char **names, char *str)
{
  int i;

  for (i = 0; names[i]; i++)
    if (strcmp (names[i], str) == 0)
      return 1;
  return 0;
}

int
mu_debug_format_spec(mu_stream_t str, const char *names, int showunset)
{
  int i;
  size_t cnt = 0;
  int rc = 0;
  struct mu_wordsplit ws;

  if (names)
    {
      ws.ws_delim = ";";
      if (mu_wordsplit (names, &ws,
			MU_WRDSF_DELIM|MU_WRDSF_WS|
			MU_WRDSF_NOVAR|MU_WRDSF_NOCMD))
	return errno;
    }
  
  for (i = 0; i < catcnt; i++)
    {
      if (names && !name_matches (ws.ws_wordv, cattab[i].name))
	continue;
      if (cattab[i].isset && cattab[i].level)
	{
	  if (cnt)
	    {
	      rc = mu_stream_printf(str, ";");
	      if (rc)
		break;
	    }
	  rc = mu_stream_printf(str, "%s", cattab[i].name);
	  if (rc)
	    break;
	  if (cattab[i].level != _LEVEL_ALL)
	    {
	      int j;
	      int delim = '.';
	      
	      for (j = MU_DEBUG_ERROR; j <= MU_DEBUG_PROT; j++)
		if (cattab[i].level & MU_DEBUG_LEVEL_MASK(j))
		  {
		    rc = mu_stream_printf(str, "%c%s", delim,
					  mu_debug_level_str[j]);
		    if (rc)
		      break;
		    delim = ',';
		  }
	    }
	  cnt++;
	}
      else if (showunset)
	{
	  rc = mu_stream_printf(str, "!%s", cattab[i].name);
	  if (rc)
	    break;
	  cnt++;
	}
    }
  if (names)
    mu_wordsplit_free (&ws);
  return rc;
}



void
mu_debug_log (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  mu_stream_printf (mu_strerr, "\033s<%d>", MU_LOG_DEBUG);
  mu_stream_vprintf (mu_strerr, fmt, ap);
  mu_stream_write (mu_strerr, "\n", 1, NULL);
  va_end (ap);
}

void
mu_debug_log_begin (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  mu_stream_printf (mu_strerr, "\033s<%d>", MU_LOG_DEBUG);
  mu_stream_vprintf (mu_strerr, fmt, ap);
  va_end (ap);
}

void
mu_debug_log_cont (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  mu_stream_vprintf (mu_strerr, fmt, ap);
  va_end (ap);
}

void
mu_debug_log_end (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  mu_stream_vprintf (mu_strerr, fmt, ap);
  mu_stream_write (mu_strerr, "\n", 1, NULL);
  va_end (ap);
}

void
mu_debug_log_nl ()
{
  mu_stream_write (mu_strerr, "\n", 1, NULL);
}
