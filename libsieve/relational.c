/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#include <unistd.h>  
#include <string.h>  
#include <sieve.h>

#define CAT2(a,b) a##b
#define CAT3(a,b,c) a##b##c

#define DCL(name,op)\
 static int CAT2(op_,name) (int num, int lim) { return num op lim; }\
 static int CAT3(op_,name,_size_t) (size_t num, size_t lim) { return num op lim; }

DCL(eq,==)
DCL(ne,!=)
DCL(gt,>)
DCL(ge,>=)
DCL(lt,<)
DCL(le,<=)

static struct reltest_tab {
  char *name;
  sieve_relcmp_t test;
  sieve_relcmpn_t stest;
} testtab[] = {
#define DEF(name) { #name, CAT2(op_,name), CAT3(op_,name,_size_t) }

  DEF(eq),
  DEF(ne),
  DEF(gt),
  DEF(ge),
  DEF(lt),
  DEF(le)
};

static struct reltest_tab *
_relcmp_lookup (const char *str)
{
  int i;

  for (i = 0; i < sizeof(testtab) / sizeof(testtab[0]); i++)
    if (strcmp (testtab[i].name, str) == 0)
      return &testtab[i];
  return 0;
}

int
sieve_str_to_relcmp (const char *str,
		      sieve_relcmp_t *test, sieve_relcmpn_t *stest)
{
  struct reltest_tab *t = _relcmp_lookup (str);
  if (t)
    {
      if (test)
	*test = t->test;
      if (stest)
	*stest = t->stest;
      return 0;
    }
  return 1;
}

sieve_relcmp_t
sieve_get_relcmp (sieve_machine_t mach, list_t tags)
{
  sieve_value_t *arg;
  sieve_relcmp_t test = NULL;
  
  if (sieve_tag_lookup (tags, "value", &arg) == 0)
    return op_ne;
  sieve_str_to_relcmp (arg->v.string, &test, NULL);
  return test;
}

int
sieve_require_relational (sieve_machine_t mach, const char *name)
{
  /* noop */
  return 0;
}
