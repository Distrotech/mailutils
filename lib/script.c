/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2002, 2005, 2007, 2009-2012, 2014-2016 Free
   Software Foundation, Inc.

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

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include "muscript.h"
#include "muscript_priv.h"

int mu_script_debug_sieve;
int mu_script_debug_guile;
int mu_script_sieve_log;

struct mu_script_fun *script_tab[] = {
  &mu_script_sieve,
#ifdef WITH_PYTHON
  &mu_script_python,  
#endif
#ifdef WITH_GUILE
  &mu_script_scheme,
#endif
  NULL
};

int
mu_script_debug_flags (const char *arg, char **endp)
{
  mu_debug_level_t lev;
    
  for (; *arg; arg++)
    {
      switch (*arg)
	{
	case 'g':
	  mu_script_debug_guile = 1;
	  break;

	case 't':
	  mu_debug_get_category_level (mu_sieve_debug_handle, &lev);
	  mu_debug_set_category_level (mu_sieve_debug_handle,
			       lev | MU_DEBUG_LEVEL_MASK(MU_DEBUG_TRACE4));
	  break;
	  
	case 'i':
	  mu_debug_get_category_level (mu_sieve_debug_handle, &lev);
	  mu_debug_set_category_level (mu_sieve_debug_handle,
			       lev | MU_DEBUG_LEVEL_MASK(MU_DEBUG_TRACE9));
	  break;
	  
	case 'l':
	  mu_script_sieve_log = 1;
	  break;
	  
	default:
	  if (endp)
	    *endp = (char*) arg;
	  return 1;
	}
    }
  return 0;
}

mu_script_t
mu_script_lang_handler (const char *lang)
{
  int i;

  for (i = 0; script_tab[i]; i++)
    if (strcmp (script_tab[i]->lang, lang) == 0)
      return script_tab[i];
  return NULL;
}

mu_script_t
mu_script_suffix_handler (const char *name)
{
  int i;
  char *suf;
  
  suf = strrchr (name, '.');
  if (!suf)
    return NULL;
  suf++;
  
  for (i = 0; script_tab[i]; i++)
    {
      char *s;

      for (s = script_tab[i]->suf; *s; s += strlen (s) + 1)
	if (strcmp (s, suf) == 0)
	  return script_tab[i];
    }
  return NULL;
}

int
mu_script_init (mu_script_t scr, const char *name, const char **env,
		mu_script_descr_t *pdescr)
{
  return scr->script_init ? scr->script_init (name, env, pdescr) : 0;
}

int
mu_script_done (mu_script_t scr, mu_script_descr_t descr)
{
  return scr->script_done ? scr->script_done (descr) : 0;
}

void
mu_script_log_enable (mu_script_t scr, mu_script_descr_t descr,
		      const char *name, const char *hdr)
{
  if (scr->script_log_enable)
    scr->script_log_enable (descr, name, hdr);
}

int
mu_script_process_msg (mu_script_t scr, mu_script_descr_t descr,
		       mu_message_t msg)
{
  return scr->script_process (descr, msg);
}

