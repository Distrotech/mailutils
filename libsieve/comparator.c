/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils; if not, write to the Free Software
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

int
sieve_register_comparator (sieve_machine_t mach,
			   const char *name,
			   int required,
			   sieve_comparator_t is, sieve_comparator_t contains,
			   sieve_comparator_t matches,
			   sieve_comparator_t regex,
			   sieve_comparator_t eq)
{
  sieve_comparator_record_t *rp;

  if (!mach->comp_list)
    {
      int rc = list_create (&mach->comp_list);
      if (rc)
	return rc;
    }

  rp = sieve_malloc (mach, sizeof (*rp));
  rp->required = required;
  rp->name = name;
  rp->comp[MU_SIEVE_MATCH_IS] = is;       
  rp->comp[MU_SIEVE_MATCH_CONTAINS] = contains; 
  rp->comp[MU_SIEVE_MATCH_MATCHES] = matches;  
  rp->comp[MU_SIEVE_MATCH_REGEX] = regex;    
  rp->comp[MU_SIEVE_MATCH_EQ] = eq;    

  return list_append (mach->comp_list, rp);
}

sieve_comparator_record_t *
_lookup (list_t list, const char *name)
{
  iterator_t itr;
  sieve_comparator_record_t *reg;

  if (!list || iterator_create (&itr, list))
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
sieve_require_comparator (sieve_machine_t mach, const char *name)
{
  sieve_comparator_record_t *reg = _lookup (mach->comp_list, name);
  if (!reg)
    {
      if (!(sieve_load_ext (mach, name) == 0
	    && (reg = _lookup (mach->comp_list, name)) != NULL))
	return 1;
    }

  reg->required = 1;
  return 0;
}

sieve_comparator_t 
sieve_comparator_lookup (sieve_machine_t mach, const char *name, int matchtype)
{
  sieve_comparator_record_t *reg = _lookup (mach->comp_list, name);
  if (reg && reg->comp[matchtype])
    return reg->comp[matchtype];
  return NULL;
}

static int
_find_comparator (void *item, void *data)
{
  sieve_runtime_tag_t *tag = item;

  if (strcmp (tag->tag, TAG_COMPFUN) == 0)
    {
      *(sieve_comparator_t*)data = tag->arg->v.ptr;
      return 1;
    }
  return 0;
}

sieve_comparator_t
sieve_get_comparator (sieve_machine_t mach, list_t tags)
{
  sieve_comparator_t comp = NULL;

  list_do (tags, _find_comparator, &comp);
  return comp ? comp : sieve_comparator_lookup (mach,
						"i;ascii-casemap",
						MU_SIEVE_MATCH_IS);
}

/* Compile time support */

struct regex_data {
  int flags;
  list_t list;
};

#ifndef FNM_CASEFOLD
static int
_pattern_upcase (void *item, void *data)
{
  char *p;

  for (p = item; *p; p++)
    *p = toupper (*p);
  return 0;
}
#endif

static int
_regex_compile (void *item, void *data)
{
  struct regex_data *rd = data;
  int rc;
  regex_t *preg = sieve_malloc (sieve_machine, sizeof (*preg));
  
  rc = regcomp (preg, (char*)item, rd->flags);
  if (rc)
    {
      size_t size = regerror (rc, preg, NULL, 0);
      char *errbuf = malloc (size + 1);
      if (errbuf)
	{
	  regerror (rc, preg, errbuf, size);
	  sieve_compile_error (sieve_filename, sieve_line_num,
			       _("regex error: %s"), errbuf);
	  free (errbuf);
	}
      else
	 sieve_compile_error (sieve_filename, sieve_line_num,
			      _("regex error"));
      return rc;
    }

  list_append (rd->list, preg);
  
  return 0;
}

static int
_free_regex (void *item, void *unused)
{
  regfree ((regex_t*)item);
  return 0;
}

static void
_free_reglist (void *data)
{
  list_t list = data;
  list_do (list, _free_regex, NULL);
  list_destroy (&list);
}

static int
comp_false (void *pattern, void *text)
{
  return 0;
}

int
sieve_match_part_checker (const char *name, list_t tags, list_t args)
{
  iterator_t itr;
  sieve_runtime_tag_t *match = NULL;
  sieve_runtime_tag_t *comp = NULL;
  sieve_runtime_tag_t *tmp;
  sieve_comparator_t compfun = NULL;
  char *compname;
  
  int matchtype;
  int err = 0;
  
  if (!tags || iterator_create (&itr, tags))
    return 0;

  for (iterator_first (itr); !err && !iterator_is_done (itr);
       iterator_next (itr))
    {
      sieve_runtime_tag_t *t;
      iterator_current (itr, (void **)&t);
      
      if (strcmp (t->tag, "is") == 0
	  || strcmp (t->tag, "contains") == 0
	  || strcmp (t->tag, "matches") == 0
	  || strcmp (t->tag, "regex") == 0
	  || strcmp (t->tag, "count") == 0
	  || strcmp (t->tag, "value") == 0)
	{
	  if (match)
	    {
	      sieve_compile_error (sieve_filename, sieve_line_num,
			     _("match type specified twice in call to `%s'"),
				   name);
	      err = 1;
	    }
	  else
	    match = t;
	}
      else if (strcmp (t->tag, "comparator") == 0) 
	comp = t;
    }

  iterator_destroy (&itr);

  if (err)
    return 1;

  if (!match || strcmp (match->tag, "is") == 0)
    matchtype = MU_SIEVE_MATCH_IS;
  else if (strcmp (match->tag, "contains") == 0)
    matchtype = MU_SIEVE_MATCH_CONTAINS;
  else if (strcmp (match->tag, "matches") == 0)
    matchtype = MU_SIEVE_MATCH_MATCHES;
  else if (strcmp (match->tag, "regex") == 0)
    matchtype = MU_SIEVE_MATCH_REGEX;
  else
    {
      char *str = match->arg->v.string;
      if (strcmp (match->tag, "count") == 0)
	{
	  sieve_value_t *val;
	  char *str;
	  size_t count;
	  
	  if (comp && strcmp (comp->arg->v.string, "i;ascii-numeric"))
	    {
	      sieve_compile_error (sieve_filename, sieve_line_num,
			   _("comparator %s is incompatible with :count in call to `%s'"),
				   comp->arg->v.string,
				   name);
	      return 1;
	    }

	  compfun = comp_false;
	  val = sieve_value_get (args, 1);
	  if (!val)
	    return 1; /* shouldn't happen */
	  /* NOTE: Type of v is always SVT_STRING_LIST */
	  list_count (val->v.list, &count);
	  if (count > 1)
	    {
	      sieve_compile_error (sieve_filename, sieve_line_num,
			_("second argument must be a list of one element"));
	      return 1;
	    }
	  list_get (val->v.list, 0, (void **) &str);
	  count = strtoul (str, &str, 10);
	  if (*str)
	    {
	      sieve_compile_error (sieve_filename, sieve_line_num,
			   _("second argument cannot be converted to number"));
	      return 1;
	    }
	}
      else
	matchtype = MU_SIEVE_MATCH_EQ;

      if (sieve_str_to_relcmp (str, NULL, NULL))
	{
	  sieve_compile_error (sieve_filename, sieve_line_num,
			       _("invalid relational match `%s' in call to `%s'"),
			       str, name);
	  return 1;
	}
    }

  if (!compfun)
    {
      compname = comp ? comp->arg->v.string : "i;ascii-casemap";
      compfun = sieve_comparator_lookup (sieve_machine, compname, matchtype);
      if (!compfun)
	{
	  sieve_compile_error (sieve_filename, sieve_line_num,
			   _("comparator `%s' is incompatible with match type `%s' in call to `%s'"),
			       compname, match ? match->tag : "is", name);
	  return 1;
	}
    }

  tmp = sieve_malloc (sieve_machine, sizeof (*tmp));
  tmp->tag = TAG_COMPFUN;
  tmp->arg = sieve_value_create (SVT_POINTER, compfun);
  list_append (tags, tmp);
  
  if (matchtype == MU_SIEVE_MATCH_REGEX)
    {
      /* To speed up things, compile all patterns at once.
	 Notice that it is supposed that patterns are in arg 2 */
      sieve_value_t *val, *newval;
      struct regex_data rd;
      int rc;
      
      if (list_get (args, 1, (void**)&val))
	return 0;

      if (strcmp (compname, "i;ascii-casemap") == 0)
	rd.flags = REG_ICASE;
      else
	rd.flags = 0;

      list_create (&rd.list);
      
      rc = sieve_vlist_do (val, _regex_compile, &rd);

      sieve_machine_add_destructor (sieve_machine, _free_reglist, rd.list);

      if (rc)
	return rc;
      newval = sieve_value_create (SVT_STRING_LIST, rd.list);
      list_replace (args, val, newval);
    }
#ifndef FNM_CASEFOLD
  else if (matchtype == MU_SIEVE_MATCH_MATCHES
	   && strcmp (compname, "i;ascii-casemap") == 0)
    {
      int rc;
      sieve_value_t *val;

      if (list_get (args, 1, (void**)&val))
	return 0;
      rc = sieve_vlist_do (val, _pattern_upcase, NULL);
      if (rc)
	return rc;
    }
#endif
  return 0;
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

static int
i_octet_regex (const char *pattern, const char *text)
{
  return regexec ((regex_t *) pattern, text, 0, NULL, 0) == 0;
}

static int
i_octet_eq (const char *pattern, const char *text)
{
  return strcmp (text, pattern);
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

  if ((b = U (*(needle = (const unsigned char*)pattern))))
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
#ifdef FNM_CASEFOLD
  return fnmatch (pattern, text, FNM_CASEFOLD) == 0;
#else
  int rc;
  char *p = strdup (text);
  _pattern_upcase (p, NULL);
  rc = fnmatch (pattern, p, 0) == 0;
  free (p);
  return rc;
#endif
}

static int
i_ascii_casemap_regex (const char *pattern, const char *text)
{
  return regexec ((regex_t *) pattern, text, 0, NULL, 0) == 0;
}

static int
i_ascii_casemap_eq (const char *pattern, const char *text)
{
  return strcasecmp (text, pattern);
}

/* :comparator i;ascii-numeric */
static int
i_ascii_numeric_is (const char *pattern, const char *text)
{
  if (isdigit ((int) *pattern))
    {
      if (isdigit ((int) *text))
	return strtol (pattern, NULL, 10) == strtol (text, NULL, 10);
      else 
	return 0;
    }
  else if (isdigit ((int) *text))
    return 0;
  else
    return 1;
}

static int
i_ascii_numeric_eq (const char *pattern, const char *text)
{
  if (isdigit ((int) *pattern))
    {
      if (isdigit ((int) *text))
	{
	  size_t a = strtoul (pattern, NULL, 10);
	  size_t b = strtoul (text, NULL, 10);
	  if (b > a)
	    return 1;
	  else if (b < a)
	    return -1;
	  return 0;
	}
      else 
	return 1;
    }
  else
    return 1;
}

void
sieve_register_standard_comparators (sieve_machine_t mach)
{
  sieve_register_comparator (mach,
			     "i;octet",
			     1,
			     i_octet_is,
			     i_octet_contains,
			     i_octet_matches,
			     i_octet_regex,
			     i_octet_eq);
  sieve_register_comparator (mach,
			     "i;ascii-casemap",
			     1,
			     i_ascii_casemap_is,
			     i_ascii_casemap_contains,
			     i_ascii_casemap_matches,
			     i_ascii_casemap_regex,
			     i_ascii_casemap_eq);
  sieve_register_comparator (mach,
			     "i;ascii-numeric",
			     0,
			     i_ascii_numeric_is,
			     NULL,
			     NULL,
			     NULL,
			     i_ascii_numeric_eq);
}
