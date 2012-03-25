/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

/* Global MH state. */

#include <mh.h>

static const char *current_folder = NULL;
int rcpt_mask = RCPT_DEFAULT;
int mh_auto_install = 1;

mu_property_t
mh_read_property_file (char *name, int ro)
{
  mu_property_t prop;
  struct mu_mh_prop *mhprop;
  int rc;
  
  mhprop = mu_zalloc (sizeof (mhprop[0]));
  mhprop->filename = name;
  mhprop->ro = ro;
  rc = mu_property_create_init (&prop, mu_mh_property_init, mhprop);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_create_init", name, rc);
      exit (1);
    }
  return prop;
}

static int
prop_merger (const char *field, const char *value, void *data)
{
  mu_property_t dst = data;
  return mu_property_set_value (dst, field, value, 1);
}

void
mh_property_merge (mu_property_t dst, mu_property_t src)
{
  int rc;
  
  if (!src)
    return;
  rc = mu_mhprop_iterate (src, prop_merger, dst);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mhprop_iterate", NULL, rc);
      exit (1);
    }
}
  
/* Global profile */

void
_mh_init_global_context ()
{
  char *p, *ctx_name;
  
  if (mu_mh_context)
    return;
  p = getenv ("CONTEXT");
  if (!p)
    p = MH_CONTEXT_FILE;
  ctx_name = mh_expand_name (NULL, p, 0);

  mu_mh_context = mh_read_property_file (ctx_name, 0);
  
  if (!current_folder)
    current_folder = mh_global_context_get ("Current-Folder",
					    mh_global_profile_get ("Inbox",
								   "inbox"));
}

void
mh_read_profile ()
{
  char *p;
  const char *fallback;
  
  p = getenv ("MH");
  if (p)
    p = mu_tilde_expansion (p, MU_HIERARCHY_DELIMITER, NULL);
  else
    {
      char *home = mu_get_homedir ();
      if (!home)
	abort (); /* shouldn't happen */
      p = mh_safe_make_file_name (home, MH_USER_PROFILE);
      free (home);
    }

  if (mh_auto_install && access (p, R_OK))
    mh_install (p, 1);

  mu_mh_profile = mh_read_property_file (p, 0);

  mu_set_folder_directory (mh_get_dir ());

  mh_set_reply_regex (mh_global_profile_get ("Reply-Regex", NULL));
  fallback = mh_global_profile_get ("Decode-Fallback", NULL);
  if (fallback && mu_set_default_fallback (fallback))
    mu_error (_("Incorrect value for decode-fallback"));

  _mh_init_global_context ();
}

/* Global context */

const char *
mh_current_folder ()
{
  return mh_global_context_get ("Current-Folder",
				mh_global_profile_get ("Inbox", "inbox"));
}

const char *
mh_set_current_folder (const char *val)
{
  int rc = mh_global_context_set ("Current-Folder", val);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mh_global_context_set",
		       "Current-Folder", rc);
      exit (1);
    }
  current_folder = mh_current_folder ();
  return current_folder;
}

/* Global sequences */
mu_property_t
mh_mailbox_get_property (mu_mailbox_t mbox)
{
  mu_property_t prop;
  int rc = mu_mailbox_get_property (mbox, &prop);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_mailbox_get_property", NULL, rc);
      exit (1);
    }
  return prop;
}
  
void
mh_global_sequences_drop (mu_mailbox_t mbox)
{
  mu_property_t prop = mh_mailbox_get_property (mbox);
  int rc = mu_property_clear (prop);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_clear", NULL, rc);
      exit (1);
    }
}

const char *
mh_global_sequences_get (mu_mailbox_t mbox, const char *name,
			 const char *defval)
{
  mu_property_t prop = mh_mailbox_get_property (mbox);
  const char *s;
  int rc = mu_property_sget_value (prop, name, &s);
  if (rc == MU_ERR_NOENT)
    s = defval;
  else if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_sget_value", name, rc);
      exit (1);
    }
  return s;
}

void
mh_global_sequences_set (mu_mailbox_t mbox, const char *name,
			 const char *value)
{
  mu_property_t prop = mh_mailbox_get_property (mbox);
  int rc = mu_property_set_value (prop, name, value, 1);
  if (rc && !(!value && rc == MU_ERR_NOENT))
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_set_value", name, rc);
      exit (1);
    }
}

void
mh_global_sequences_iterate (mu_mailbox_t mbox,
                             mu_mhprop_iterator_t fp, void *data)
{
  int rc;
  mu_iterator_t itr;
  mu_property_t prop = mh_mailbox_get_property (mbox);

  rc = mu_property_get_iterator (prop, &itr);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_property_get_iterator", NULL, rc);
      exit (1);
    }
  mu_mhprop_iterate (prop, fp, data);
}

/* Global state */

void
mh_global_save_state ()
{
  int rc;
  
  mh_global_context_set ("Current-Folder", current_folder);
  rc = mu_property_save (mu_mh_context);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_profile_save", "context", rc);
      exit (1);
    }
}


