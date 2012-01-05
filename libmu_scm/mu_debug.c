/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#include "mu_scm.h"
#include <mailutils/iterator.h>
#include <mailutils/stdstream.h>

SCM_DEFINE_PUBLIC (scm_mu_debug_parse, "mu-debug-parse", 1, 0, 0,
		   (SCM spec),
"Parses @var{spec} and sets MU debugging level according to it.")
#define FUNC_NAME s_scm_mu_debug_parse
{
  char *s;
  
  SCM_ASSERT (scm_is_string (spec), spec, SCM_ARG1, FUNC_NAME);
  s = scm_to_locale_string (spec);
  mu_debug_parse_spec (s);
  free (s);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

static int
string_member_p (SCM list, SCM x)
{
  return scm_member (x, list) != SCM_BOOL_F;
} 

SCM_DEFINE_PUBLIC (scm_mu_debug_set, "mu-debug-set", 1, 0, 0,
		   (SCM catlist),
"Sets MU debug level according to @var{catlist}, which is a list of\n"
"conses: (cons category level)\n")  
#define FUNC_NAME s_scm_mu_debug_set
{
  SCM_ASSERT (scm_is_pair (catlist), catlist, SCM_ARG1, FUNC_NAME);
  for (; !scm_is_null (catlist); catlist = SCM_CDR (catlist))
    {
      SCM cell = SCM_CAR (catlist);
      SCM x;
      char *name;
      mu_debug_level_t lev;

      SCM_ASSERT (scm_is_pair (cell), cell, SCM_ARGn, FUNC_NAME);
      x = scm_car (cell);
      SCM_ASSERT (scm_is_string (x), x, SCM_ARGn, FUNC_NAME);
      name = scm_to_locale_string (x);
      x = scm_cdr (cell);
      SCM_ASSERT (scm_is_integer (x), x, SCM_ARGn, FUNC_NAME);
      lev = scm_to_int (x);
      if (lev == 0)
	mu_debug_disable_category (name, strlen (name));
      else
	mu_debug_enable_category (name, strlen (name), lev);
      free (name);
    }      
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE_PUBLIC (scm_mu_debug_get, "mu-debug-get", 0, 2, 0,
		   (SCM catlist, SCM skipunset),
"Returns a list of MU debugging categories with corresponding levels.\n"
"If @var{catlist} is supplied, it is a list of category names.  In this case\n"
"only categories from this list are returned")
#define FUNC_NAME s_scm_mu_debug_get
{
  int skipunset_flag = 0;
  mu_iterator_t itr;
  SCM head = SCM_EOL, tail = SCM_EOL;
  int (*member_p) (SCM list, SCM needle);

  if (SCM_UNBNDP (catlist) || catlist == SCM_BOOL_F)
    member_p = NULL;
  else
    {
      SCM_ASSERT (scm_is_pair (catlist), catlist, SCM_ARG1, FUNC_NAME);
      member_p = string_member_p;
    }
  
  if (!SCM_UNBNDP (skipunset))
    {
      SCM_ASSERT (scm_is_bool (skipunset), skipunset, SCM_ARG2, FUNC_NAME);
      skipunset_flag = skipunset == SCM_BOOL_T;
    }

  mu_debug_get_iterator (&itr, skipunset_flag);
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      const char *name;
      mu_debug_level_t level;
      SCM scm_name;
      
      mu_iterator_current_kv (itr, (const void**) &name, (void**) &level);
      scm_name = scm_from_locale_string (name);
      if (!member_p || member_p (catlist, scm_name))
	{
	  SCM scm_new = scm_cons (scm_cons (scm_name,
					    scm_from_uintmax (level)),
				  SCM_EOL);

	  if (scm_is_null (head))
	    head = tail = scm_new;
	  else
	    {
	      SCM_SETCDR (tail, scm_new);
	      tail = scm_new;
	    }
	}
    }
  mu_iterator_destroy (&itr);
  return head;
}
#undef FUNC_NAME

void
mu_scm_debug_init ()
{
#include "mu_debug.x"
}
