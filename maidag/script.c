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

#include "maidag.h"

int
script_register (const char *pattern)
{
  mu_script_t scr;
  struct maidag_script *p;
  
  if (script_handler)
    scr = script_handler;
  else
    {
      scr = mu_script_suffix_handler (pattern);
      if (!scr)
	return EINVAL;
    }

  p = malloc (sizeof (*p));
  if (!p)
    return MU_ERR_FAILURE;
  
  p->scr = scr;
  p->pat = pattern;

  if (!script_list)
    {
      if (mu_list_create (&script_list))
	return MU_ERR_FAILURE;
    }

  if (mu_list_append (script_list, p))
    return MU_ERR_FAILURE;

  return 0;
}


struct apply_script_closure
{
  struct mu_auth_data *auth;
  mu_message_t msg;
};

static char const *script_env[] = { "location=MDA", "phase=during", NULL };

static int
apply_script (void *item, void *data)
{
  struct maidag_script *scr = item;
  struct apply_script_closure *clos = data;
  char *progfile;
  int rc;
  struct stat st;
  mu_script_descr_t sd;

  progfile = mu_expand_path_pattern (scr->pat, clos->auth->name);
  if (stat (progfile, &st))
    {
      if (debug_level > 2)
	mu_diag_funcall (MU_DIAG_DEBUG, "stat", progfile, errno);
      else if (errno != ENOENT)
	mu_diag_funcall (MU_DIAG_NOTICE, "stat", progfile, errno);
      free (progfile);
      return 0;
    }

  rc = mu_script_init (scr->scr, progfile, script_env, &sd);
  if (rc)
    mu_error (_("initialization of script %s failed: %s"),
	      progfile, mu_strerror (rc));
  else
    {
      if (sieve_enable_log)
	mu_script_log_enable (scr->scr, sd, clos->auth->name,
			      message_id_header);
      rc = mu_script_process_msg (scr->scr, sd, clos->msg);
      if (rc)
	mu_error (_("script %s failed: %s"), progfile, mu_strerror (rc));
      mu_script_done (scr->scr, sd);
    }

  free (progfile);

  return rc;
}
  
int
script_apply (mu_message_t msg, struct mu_auth_data *auth)
{
  int rc = 0;
  
  if (script_list)
    {
      mu_attribute_t attr;
      struct apply_script_closure clos;

      clos.auth = auth;
      clos.msg = msg;

      mu_message_get_attribute (msg, &attr);
      mu_attribute_unset_deleted (attr);
      if (switch_user_id (auth, 1) == 0)
	{
	  chdir (auth->dir);
	  rc = mu_list_foreach (script_list, apply_script, &clos);
	  chdir ("/");
	  switch_user_id (auth, 0);
	  if (rc == 0)
	    {
	      mu_attribute_t attr;
	      mu_message_get_attribute (msg, &attr);
	      rc = mu_attribute_is_deleted (attr);
	    }
	  else
	    rc = 0;
  	}
    }
  return rc;
}
