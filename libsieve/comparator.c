/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve.h>
#include <fnmatch.h>
#include <regex.h>

typedef struct {
  char *name;
  int required;
  sieve_comparator_t comp[MU_SIEVE_MATCH_LAST];
} sieve_comparator_record_t;


static list_t comparator_list;

int
sieve_register_comparator (const char *name,
			   int required,
			   sieve_comparator_t is, sieve_comparator_t contains,
			   sieve_comparator_t matches,
			   sieve_comparator_t regex)
{
  sieve_comparator_record_t *rp;

  if (!comparator_list)
    {
      int rc = list_create (&comparator_list);
      if (rc)
	return rc;
    }

  rp = sieve_palloc (&sieve_machine->memory_pool, sizeof (*rp));
  rp->required = required;
  rp->name = name;
  rp->comp[MU_SIEVE_MATCH_IS] = is;       
  rp->comp[MU_SIEVE_MATCH_CONTAINS] = contains; 
  rp->comp[MU_SIEVE_MATCH_MATCHES] = matches;  
  rp->comp[MU_SIEVE_MATCH_REGEX] = regex;    

  return list_append (comparator_list, rp);
}

sieve_comparator_record_t *
_lookup (const char *name)
{
  iterator_t itr;
  sieve_comparator_record_t *reg;

  if (!comparator_list || iterator_create (&itr, comparator_list))
    return NULL;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&reg);
      if (strcmp (reg->name, name) == 0)
	break;
      else
	reg = NULL;
    }
  iterator_destroy (&itr);
  return reg;
}
    
int
sieve_require_comparator (const char *name)
{
  sieve_comparator_record_t *reg = _lookup (name);
  if (!reg)
    return 1;
  reg->required = 1;
  return 0;
}

sieve_comparator_t 
sieve_comparator_lookup (const char *name, int matchtype)
{
  sieve_comparator_record_t *reg = _lookup (name);
  if (reg && reg->comp[matchtype])
    return reg->comp[matchtype];
  return NULL;
}

sieve_comparator_t
sieve_get_comparator (list_t tags)
{
  iterator_t itr;
  char *compname = "i;ascii-casemap";
  int matchtype = MU_SIEVE_MATCH_IS;
  
  if (!tags || iterator_create (&itr, tags))
    return NULL;

  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      sieve_runtime_tag_t *t;
      iterator_current (itr, (void **)&t);
      switch (t->tag)
	{
	case TAG_COMPARATOR:
	  compname = t->arg->v.string;
	  break;
	  
	case TAG_IS:
	  matchtype = MU_SIEVE_MATCH_IS;
	  break;
	  
	case TAG_CONTAINS:
	  matchtype = MU_SIEVE_MATCH_CONTAINS;
	  break;
	  
	case TAG_MATCHES:
	  matchtype = MU_SIEVE_MATCH_MATCHES;
	  break;
	  
	case TAG_REGEX:
	  matchtype = MU_SIEVE_MATCH_REGEX;
	  break;
	}
    }
  iterator_destroy (&itr);
  return sieve_comparator_lookup (compname, matchtype);
}

/* Particular comparators */

/* :comparator i;octet */

static int
i_octet_is (const char *pattern, const char *text)
{
  return strcmp (pattern, text) == 0;
}

static int
i_octet_contains (const char *pattern, const char *text)
{
  return strstr (text, pattern) != NULL;
}

static int 
i_octet_matches (const char *pattern, const char *text)
{
  return fnmatch (pattern, text, 0) == 0;
}

/* FIXME */
static int
i_octet_regex (const char *pattern, const char *text)
{
  return regexec ((regex_t *) pattern, text, 0, NULL, 0) == 0;
}

/* :comparator i;ascii-casemap */
static int
i_ascii_casemap_is (const char *pattern, const char *text)
{
  return strcasecmp (pattern, text) == 0;
}

/* Based on strstr from GNU libc (Stephen R. van den Berg,
   berg@pool.informatik.rwth-aachen.de) */

static int
i_ascii_casemap_contains (const char *pattern, const char *text)
{
  register const unsigned char *haystack, *needle;
  register unsigned int b, c;

#define U(c) toupper (c)
  
  haystack = (const unsigned char *)text;

  if ((b = U (*(needle= (const unchar*)text))))
    {
      haystack--;		
      do
	{
	  if (!(c = *++haystack))
	    goto ret0;
	}
      while (U (c) != b);

      if (!(c = *++needle))
	goto foundneedle;

      c = U (c);
      ++needle;
      goto jin;

      for (;;)
        { 
          register unsigned int a;
	  register const unsigned char *rhaystack, *rneedle;

	  do
	    {
	      if (!(a = *++haystack))
		goto ret0;
	      if (U (a) == b)
		break;
	      if (!(a = *++haystack))
		goto ret0;
shloop:     }
          while (U (a) != b);
	  
jin:     if (!(a = *++haystack))
	    goto ret0;

	  if (U (a) != c)
	    goto shloop;

	  if (U (*(rhaystack = haystack-- + 1)) ==
	      (a = U (*(rneedle = needle))))
	    do
	      {
		if (!a)
		  goto foundneedle;
		if (U (*++rhaystack) != (a = U (*++needle)))
		  break;
		if (!a)
		  goto foundneedle;
	      }
	    while (U (*++rhaystack) == (a = U (*++needle)));

	  needle = rneedle;

	  if (!a)
	    break;
        }
    }
foundneedle:
  return 1;
ret0:
  return 0;

#undef U
}

static int
i_ascii_casemap_matches (const char *pattern, const char *text)
{
  /* Compile time should do the rest: */
  return fnmatch (pattern, text, 0) == 0;
}

static int
i_ascii_casemap_regex (const char *pattern, const char *text)
{
  return regexec ((regex_t *) pattern, text, 0, NULL, 0) == 0;
}

void
sieve_register_standard_comparators ()
{
  sieve_register_comparator ("i;octet",
			     1,
			     i_octet_is,
			     i_octet_contains,
			     i_octet_matches,
			     i_octet_regex);
  sieve_register_comparator ("i;ascii-casemap",
			     1,
			     i_ascii_casemap_is,
			     i_ascii_casemap_contains,
			     i_ascii_casemap_matches,
			     i_ascii_casemap_regex);
}
