/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Global MH state. */

#include <mh.h>

char *current_folder = NULL;
size_t current_message = 0;
mh_context_t *context;
mh_context_t *profile;
mh_context_t *sequences;

/* Global profile */

char *
mh_global_profile_get (char *name, char *defval)
{
  return mh_context_get_value (profile, name, defval);
}

int
mh_global_profile_set (const char *name, const char *value)
{
  return mh_context_set_value (profile, name, value);
}

void
mh_read_profile ()
{
  char *p;
  
  p = getenv ("MH");
  if (p)
    p = mu_tilde_expansion (p, "/", NULL);
  else
    {
      char *home = mu_get_homedir ();
      if (!home)
	abort (); /* shouldn't happen */
      asprintf (&p, "%s/%s", home, MH_USER_PROFILE);
      free (home);
    }
  profile = mh_context_create (p, 1);
  mh_context_read (profile);
}

/* Global context */

void
_mh_init_global_context ()
{
  char *p, *ctx_name;
  
  if (context)
    return;
  mu_path_folder_dir = mh_get_dir ();
  p = getenv ("CONTEXT");
  if (!p)
    p = "context";
  ctx_name = mh_expand_name (p, 0);
  context = mh_context_create (ctx_name, 1);
  mh_context_read (context);
  
  if (!current_folder)
    current_folder = mh_context_get_value (context, "Current-Folder",
					   mh_global_profile_get ("Inbox",
							          "inbox"));
}

char *
mh_global_context_get (const char *name, const char *defval)
{
  _mh_init_global_context ();
  return mh_context_get_value (context, name, defval);
}

int
mh_global_context_set (const char *name, const char *value)
{
  _mh_init_global_context ();
  return mh_context_set_value (context, name, value);
}

char *
mh_current_folder ()
{
  return mh_global_context_get ("Current-Folder",
				mh_global_profile_get ("Inbox", "inbox"));
}

/* Global sequences */

void
_mh_init_global_sequences ()
{
  char *name, *p, *seq_name;

  if (sequences)
    return;
  
  _mh_init_global_context ();
  name = mh_global_profile_get ("mh-sequences", MH_SEQUENCES_FILE);
  p = mh_expand_name (current_folder, 0);
  asprintf (&seq_name, "%s/%s", p, name);
  free (p);
  sequences = mh_context_create (seq_name, 1);
  if (mh_context_read (sequences) == 0)
    {
      p = mh_context_get_value (sequences, "cur", "0");
      current_message = strtoul (p, NULL, 10);
    }
}

char *
mh_global_sequences_get (const char *name, const char *defval)
{
  _mh_init_global_sequences ();
  return mh_context_get_value (sequences, name, defval);
}

int
mh_global_sequences_set (const char *name, const char *value)
{
  _mh_init_global_sequences ();
  return mh_context_set_value (sequences, name, value);
}

/* Global state */

void
mh_global_save_state ()
{
  char buf[64];
  snprintf (buf, sizeof buf, "%lu", (unsigned long) current_message);
  mh_context_set_value (sequences, "cur", buf);
  mh_context_write (sequences);

  mh_context_set_value (context, "Current-Folder", current_folder);
  mh_context_write (context);
}
