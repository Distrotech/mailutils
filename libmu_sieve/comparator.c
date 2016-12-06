/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2004-2005, 2007-2012, 2014-2016 Free
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  
#include <string.h>  
#include <sieve-priv.h>
#include <regex.h>
#include <mailutils/cctype.h>
#include <mailutils/cstr.h>

void
mu_sieve_register_comparator (mu_sieve_machine_t mach,
			      const char *name,
			      int required,
			      mu_sieve_comparator_t is,
			      mu_sieve_comparator_t contains,
			      mu_sieve_comparator_t matches,
			      mu_sieve_comparator_t regex,
			      mu_sieve_comparator_t eq)
{
  mu_sieve_registry_t *reg = mu_sieve_registry_add (mach, name);

  reg->type = mu_sieve_record_comparator;
  reg->required = required;
  reg->name = name;
  reg->v.comp[MU_SIEVE_MATCH_IS] = is;       
  reg->v.comp[MU_SIEVE_MATCH_CONTAINS] = contains; 
  reg->v.comp[MU_SIEVE_MATCH_MATCHES] = matches;  
  reg->v.comp[MU_SIEVE_MATCH_REGEX] = regex;    
  reg->v.comp[MU_SIEVE_MATCH_EQ] = eq;    
}

mu_sieve_comparator_t 
mu_sieve_comparator_lookup (mu_sieve_machine_t mach, const char *name, 
                            int matchtype)
{
  mu_sieve_registry_t *reg =
    mu_sieve_registry_lookup (mach, name, mu_sieve_record_comparator);
  if (reg && reg->v.comp[matchtype])
    return reg->v.comp[matchtype];
  return NULL;
}

static int i_ascii_casemap_is (mu_sieve_machine_t mach,
			       mu_sieve_string_t *pattern, const char *text);

mu_sieve_comparator_t
mu_sieve_get_comparator (mu_sieve_machine_t mach)
{
  if (!mach->comparator)
    return i_ascii_casemap_is;
  return mach->comparator;
}

/* Compile time support */
static void
compile_pattern (mu_sieve_machine_t mach, mu_sieve_string_t *pattern, int flags)
{
  int rc;
  regex_t *preg;

  if (pattern->rx)
    return;
  preg = mu_sieve_malloc (mach, sizeof (*preg));
  rc = regcomp (preg, pattern->orig, REG_EXTENDED | flags);
  if (rc)
    {
      size_t size = regerror (rc, preg, NULL, 0);
      char *errbuf = malloc (size + 1);
      if (errbuf)
	{
	  regerror (rc, preg, errbuf, size);
	  mu_sieve_error (mach, _("regex error: %s"), errbuf);
	  free (errbuf);
	}
      else
	mu_sieve_error (mach, _("regex error"));
      mu_sieve_abort (mach);
    }
  pattern->rx = preg;
}

static void
compile_wildcard (mu_sieve_machine_t mach, mu_sieve_string_t *pattern,
		  int flags)
{
  int rc;
  regex_t *preg;

  if (pattern->rx)
    return;
  preg = mu_sieve_malloc (mach, sizeof (*preg));
  rc = mu_glob_compile (preg, pattern->orig, flags);
  if (rc)
    {
      mu_sieve_error (mach, _("can't compile pattern"));
      mu_sieve_abort (mach);
    }
  pattern->rx = preg;
}

static int
comp_false (mu_sieve_machine_t mach, mu_sieve_string_t *pattern,
	    const char *text)
{
  return 0;
}

int
mu_sieve_match_part_checker (mu_sieve_machine_t mach)
{
  size_t i;
  mu_sieve_value_t *match = NULL;
  mu_sieve_comparator_t compfun = NULL;
  char *compname = NULL;
  
  int matchtype;

  if (mach->tagcount == 0)
    return 0;

  for (i = 0; i < mach->tagcount; i++)
    {
      mu_sieve_value_t *t = mu_sieve_get_tag_n (mach, i);
      
      if (strcmp (t->tag, "is") == 0
	  || strcmp (t->tag, "contains") == 0
	  || strcmp (t->tag, "matches") == 0
	  || strcmp (t->tag, "regex") == 0
	  || strcmp (t->tag, "count") == 0
	  || strcmp (t->tag, "value") == 0)
	{
	  if (match)
	    {
	      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
				_("match type specified twice in call to `%s'"),
				mach->identifier);
	      mu_i_sv_error (mach);
	      return 1;
	    }
	  else
	    match = t;
	}
      else if (strcmp (t->tag, "comparator") == 0)
	{
	  if (t->type != SVT_STRING)
	    abort ();
	  compname = mu_sieve_string (mach, &t->v.list, 0);
	}
    }

  if (!match || strcmp (match->tag, "is") == 0)
    matchtype = MU_SIEVE_MATCH_IS;
  else if (strcmp (match->tag, "contains") == 0)
    matchtype = MU_SIEVE_MATCH_CONTAINS;
  else if (strcmp (match->tag, "matches") == 0)
    matchtype = MU_SIEVE_MATCH_MATCHES;
  else if (strcmp (match->tag, "regex") == 0)
    matchtype = MU_SIEVE_MATCH_REGEX;
  else if (match->type == SVT_STRING)
    {
      char *str = mu_sieve_string (mach, &match->v.list, 0);
      if (strcmp (match->tag, "count") == 0)
	{
	  mu_sieve_value_t *val;
	  char *str;
	  
	  if (compname && strcmp (compname, "i;ascii-numeric"))
	    {
	      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus,
				/* TRANSLATORS: Do not translate ':count'.
				   It is the name of a Sieve tag */
				_("comparator %s is incompatible with "
				  ":count in call to `%s'"),
				compname,
				mach->identifier);
	      mu_i_sv_error (mach);
	      return 1;
	    }

          matchtype = MU_SIEVE_MATCH_LAST; /* to not leave it undefined */
	  compname = "false";
	  compfun = comp_false;
	  val = mu_sieve_get_arg_untyped (mach, 1);
	  /* NOTE: Type of val is always SVT_STRING_LIST */
	  if (val->type != SVT_STRING_LIST)
	    abort ();
	  if (val->v.list.count > 1)
	    {
	      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			_("second argument must be a list of one element"));
	      mu_i_sv_error (mach);
	      return 1;
	    }
	  str = mu_sieve_string_raw (mach, &val->v.list, 0)->orig;
	  str = mu_str_skip_class (str, MU_CTYPE_DIGIT);
	  if (*str)
	    {
	      mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			   _("second argument cannot be converted to number"));
	      mu_i_sv_error (mach);
	      return 1;
	    }
	}
      else
	matchtype = MU_SIEVE_MATCH_EQ;

      if (mu_sieve_str_to_relcmp (str, NULL, NULL))
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			    _("invalid relational match `%s' in call to `%s'"),
			    str, mach->identifier);
	  mu_i_sv_error (mach);
	  return 1;
	}
    }
  else
    abort ();//FIXME
  
  if (!compfun)
    {
      if (!compname)
	compname = "i;ascii-casemap";
      compfun = mu_sieve_comparator_lookup (mach, compname, matchtype);
      if (!compfun)
	{
	  mu_diag_at_locus (MU_LOG_ERROR, &mach->locus, 
			    _("comparator `%s' is incompatible with match type `%s' in call to `%s'"),
			    compname, match ? match->tag : "is",
			    mach->identifier);
	  mu_i_sv_error (mach);
	  return 1;
	}
    }

  mach->comparator = compfun;
  
  return 0;
}

/* Particular comparators */

/* :comparator i;octet */

static int
i_octet_is (mu_sieve_machine_t mach, mu_sieve_string_t *pattern,
	    const char *text)
{
  return strcmp (pattern->orig, text) == 0;
}

static int
i_octet_contains (mu_sieve_machine_t mach, mu_sieve_string_t *pattern,
		  const char *text)
{
  return strstr (text, pattern->orig) != NULL;
}

static int 
i_octet_matches (mu_sieve_machine_t mach, mu_sieve_string_t *pattern,
		 const char *text)
{
  compile_wildcard (mach, pattern, 0);
  return regexec ((regex_t *)pattern->rx, text, 0, NULL, 0) == 0;
}

static int
i_octet_regex (mu_sieve_machine_t mach, mu_sieve_string_t *pattern,
	       const char *text)
{
  compile_pattern (mach, pattern, 0);
  return regexec ((regex_t *)pattern->rx, text, 0, NULL, 0) == 0;
}

static int
i_octet_eq (mu_sieve_machine_t mach,
	    mu_sieve_string_t *pattern, const char *text)
{
  return strcmp (text, pattern->orig);
}

/* :comparator i;ascii-casemap */
static int
i_ascii_casemap_is (mu_sieve_machine_t mach,
		    mu_sieve_string_t *pattern, const char *text)
{
  return mu_c_strcasecmp (pattern->orig, text) == 0;
}

static int
i_ascii_casemap_contains (mu_sieve_machine_t mach,
			  mu_sieve_string_t *pattern, const char *text)
{
  return mu_c_strcasestr (text, pattern->orig) != NULL;
}

static int
i_ascii_casemap_matches (mu_sieve_machine_t mach,
			 mu_sieve_string_t *pattern, const char *text)
{
  compile_wildcard (mach, pattern, MU_GLOBF_ICASE);
  return regexec ((regex_t *)pattern->rx, text, 0, NULL, 0) == 0;
}

static int
i_ascii_casemap_regex (mu_sieve_machine_t mach,
		       mu_sieve_string_t *pattern, const char *text)
{
  compile_pattern (mach, pattern, REG_ICASE);
  return regexec ((regex_t *) pattern->rx, text, 0, NULL, 0) == 0;
}

static int
i_ascii_casemap_eq (mu_sieve_machine_t mach,
		    mu_sieve_string_t *pattern, const char *text)
{
  return mu_c_strcasecmp (text, pattern->orig);
}

/* :comparator i;ascii-numeric */
static int
i_ascii_numeric_is (mu_sieve_machine_t mach,
		    mu_sieve_string_t *pattern, const char *text)
{
  if (mu_isdigit (*pattern->orig))
    {
      if (mu_isdigit (*text))
	//FIXME: Error checking
	return strtol (pattern->orig, NULL, 10) == strtol (text, NULL, 10);
      else 
	return 0;
    }
  else if (mu_isdigit (*text))
    return 0;
  else
    return 1;
}

static int
i_ascii_numeric_eq (mu_sieve_machine_t mach,
		    mu_sieve_string_t *pattern, const char *text)
{
  if (mu_isdigit (*pattern->orig))
    {
      if (mu_isdigit (*text))
	{
	  size_t a = strtoul (pattern->orig, NULL, 10);
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
mu_i_sv_register_standard_comparators (mu_sieve_machine_t mach)
{
  mu_sieve_register_comparator (mach,
			     "i;octet",
			     1,
			     i_octet_is,
			     i_octet_contains,
			     i_octet_matches,
			     i_octet_regex,
			     i_octet_eq);
  mu_sieve_register_comparator (mach,
			     "i;ascii-casemap",
			     1,
			     i_ascii_casemap_is,
			     i_ascii_casemap_contains,
			     i_ascii_casemap_matches,
			     i_ascii_casemap_regex,
			     i_ascii_casemap_eq);
  mu_sieve_register_comparator (mach,
			     "i;ascii-numeric",
			     0,
			     i_ascii_numeric_is,
			     NULL,
			     NULL,
			     NULL,
			     i_ascii_numeric_eq);
}
