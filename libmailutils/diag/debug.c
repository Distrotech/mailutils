/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007-2008, 2010-2012 Free
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
#include <mailutils/iterator.h>
#include <mailutils/cstr.h>

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

int
mu_debug_set_category_level (mu_debug_handle_t catn, mu_debug_level_t level)
{
  if (catn < catcnt)
    {
      cattab[catn].isset = 1;
      cattab[catn].level = level;
      return 0;
    }
  return MU_ERR_NOENT;
}
  
int
mu_debug_get_category_level (mu_debug_handle_t catn, mu_debug_level_t *plev)
{
  if (catn < catcnt)
    {
      if (!cattab[catn].isset)
	*plev = 0;
      else
	*plev = cattab[catn].level;
      return 0;
    }
  return MU_ERR_NOENT;
}

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
mu_debug_level_from_string (const char *str, mu_debug_level_t *lev,
			    char **endp)
{
  int i;
  const char *p;
  char *q;
    
  for (i = 0; i < MU_ARRAY_SIZE (mu_debug_level_str); i++)
    {
      for (p = str, q = mu_debug_level_str[i]; ; p++, q++)
	{
	  if (!*q)
	    {
	      if (endp)
		*endp = (char*) p;
	      *lev = i;
	      return 0;
	    }
	  if (*q != *p)
	    break;
	}
    }
  return MU_ERR_NOENT;
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
	      mu_debug_level_t lev = 0;
	      mu_debug_level_t xlev = 0;
	      char *end;
	      
	      for (i = 0; i < ws.ws_wordc; i++)
		{
		  char *s = ws.ws_wordv[i];
		  int exact = 0;
		  mu_debug_level_t n;
		  mu_debug_level_t *tgt = &lev;
		  
		  if (*s == '!')
		    {
		      if (i == 0)
			lev = MU_DEBUG_LEVEL_UPTO (MU_DEBUG_PROT);
		      tgt = &xlev;
		      s++;
		    }
		  if (*s == '=')
		    {
		      exact = 1;
		      s++;
		    }

		  if (mu_debug_level_from_string (s, &n, &end))
		    {
		      mu_error (_("unknown level `%s'"), s);
		      continue;
		    }
		  else if (*end == '-')
		    {
		      mu_debug_level_t l;
		      
		      s = end + 1;
		      if (mu_debug_level_from_string (s, &l, &end))
			{
			  mu_error (_("unknown level `%s'"), s);
			  continue;
			}
		      else if (*end)
			{
			  mu_error (_("invalid level: %s"), s);
			  continue;
			}
		      if (n < l)
			*tgt |= MU_DEBUG_LEVEL_RANGE (n, l);
		      else
			*tgt |= MU_DEBUG_LEVEL_RANGE (l, n);
		    }
		  else if (*end)
		    {
		      mu_error (_("invalid level: %s"), s);
		      continue;
		    }
		  else if (exact)
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
mu_debug_format_spec (mu_stream_t str, const char *names, int showunset)
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
	  rc = mu_stream_printf (str, "%s", cattab[i].name);
	  if (rc)
	    break;
	  if (cattab[i].level != _LEVEL_ALL)
	    {
	      mu_debug_level_t j = MU_DEBUG_ERROR, minl, maxl;
	      int delim = '.';

	      while (1)
		{
		  /* Find the least bit set */
		  for (; j <= MU_DEBUG_PROT; j++)
		    if (cattab[i].level & MU_DEBUG_LEVEL_MASK (j))
		      break;

		  if (j > MU_DEBUG_PROT)
		    break;
		  
		  minl = j;

		  for (; j + 1 <= MU_DEBUG_PROT &&
			 cattab[i].level & MU_DEBUG_LEVEL_MASK (j + 1);
		       j++)
		    ;
		  
		  maxl = j++;

		  if (minl == maxl)
		    rc = mu_stream_printf (str, "%c=%s", delim,
					   mu_debug_level_str[minl]);
		  else if (minl == 0)
		    rc = mu_stream_printf (str, "%c%s", delim,
					   mu_debug_level_str[maxl]);
		  else
		    rc = mu_stream_printf (str, "%c%s-%s", delim,
					   mu_debug_level_str[minl],
					   mu_debug_level_str[maxl]);
		  if (rc)
		    break;
		  delim = ',';
		}
	    }
	  cnt++;
	}
      else if (showunset)
	{
	  if (cnt)
	    {
	      rc = mu_stream_printf(str, ";");
	      if (rc)
		break;
	    }
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


/* Iterator */

static mu_iterator_t iterator_head;

#define ITR_BACKWARDS 0x01
#define ITR_SKIPUNSET 0x02
#define ITR_FINISHED  0x04

struct debug_iterator
{
  size_t pos;
  int flags;
};

static int
first (void *owner)
{
  struct debug_iterator *itr = owner;
  itr->flags &= ~ITR_FINISHED;
  if (itr->flags & ITR_BACKWARDS)
    itr->pos = catcnt - 1;
  else
    itr->pos = 0;
  return 0;
}

static int
next (void *owner)
{
  struct debug_iterator *itr = owner;
  itr->flags &= ~ITR_FINISHED;
  do
    {
      if (itr->flags & ITR_BACKWARDS)
	{
	  if (itr->pos)
	    itr->pos--;
	  else
	    itr->flags |= ITR_FINISHED;
	}
      else
	{
	  if (itr->pos < catcnt - 1)
	    itr->pos++;
	  else
	    itr->flags |= ITR_FINISHED;
	}
    }
  while ((itr->flags & ITR_SKIPUNSET) &&
	 !(itr->flags & ITR_FINISHED) &&
	 !cattab[itr->pos].isset);

  return 0;
}

static int
getitem (void *owner, void **pret, const void **pkey)
{
  struct debug_iterator *itr = owner;
  *(mu_debug_level_t*) pret = cattab[itr->pos].level;
  if (pkey)
    *pkey = cattab[itr->pos].name;
  return 0;
}

static int
finished_p (void *owner)
{
  struct debug_iterator *itr = owner;
  return itr->flags & ITR_FINISHED;
}

static int
delitem (void *owner, void *item)
{
  struct debug_iterator *itr = owner;
  return mu_c_strcasecmp (cattab[itr->pos].name, (char *) item) == 0 ?
     MU_ITR_DELITEM_NEXT : MU_ITR_DELITEM_NOTHING;
}

static int
list_data_dup (void **ptr, void *owner)
{
  *ptr = malloc (sizeof (struct debug_iterator));
  if (*ptr == NULL)
    return ENOMEM;
  memcpy (*ptr, owner, sizeof (struct debug_iterator));
  return 0;
}

static int
list_itrctl (void *owner, enum mu_itrctl_req req, void *arg)
{
  struct debug_iterator *itr = owner;
  
  switch (req)
    {
    case mu_itrctl_tell:
      /* Return current position in the object */
      if (!arg)
	return EINVAL;
      *(size_t*)arg = itr->pos;
      break;

    case mu_itrctl_delete:
    case mu_itrctl_delete_nd:
      /* Delete current element */
      cattab[itr->pos].level = 0;
      cattab[itr->pos].isset = 0;
      break;

    case mu_itrctl_replace:
    case mu_itrctl_replace_nd:
      if (!arg)
	return EINVAL;
      cattab[itr->pos].level = *(mu_debug_level_t*)arg;
      break;

    case mu_itrctl_qry_direction:
      if (!arg)
	return EINVAL;
      else
	*(int*)arg = itr->flags & ITR_BACKWARDS;
      break;

    case mu_itrctl_set_direction:
      if (!arg)
	return EINVAL;
      else
	itr->flags |= ITR_BACKWARDS;
      break;

    default:
      return ENOSYS;
    }
  return 0;
}

int
mu_debug_get_iterator (mu_iterator_t *piterator, int skipunset)
{
  int status;
  mu_iterator_t iterator;
  struct debug_iterator *itr;
  
  itr = malloc (sizeof *itr);
  if (!itr)
    return ENOMEM;
  itr->pos = 0;
  itr->flags = skipunset ? ITR_SKIPUNSET : 0;
  status = mu_iterator_create (&iterator, itr);
  if (status)
    {
      free (itr);
      return status;
    }

  mu_iterator_set_first (iterator, first);
  mu_iterator_set_next (iterator, next);
  mu_iterator_set_getitem (iterator, getitem);
  mu_iterator_set_finished_p (iterator, finished_p);
  mu_iterator_set_delitem (iterator, delitem);
  mu_iterator_set_dup (iterator, list_data_dup);
  mu_iterator_set_itrctl (iterator, list_itrctl);
  
  mu_iterator_attach (&iterator_head, iterator);

  *piterator = iterator;
  return 0;
}


void
mu_debug_log (const char *fmt, ...)
{
  va_list ap;

  mu_diag_init ();
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

  mu_diag_init ();
  mu_stream_flush (mu_strerr);
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
