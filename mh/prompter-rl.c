/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

/* MH prompter - readline support */

#include <mh.h>
#include "prompter.h"
#include <readline/readline.h>

void
prompter_init ()
{
}

void
prompter_done ()
{
}

void
prompter_set_erase (const char *keyseq)
{
  rl_bind_keyseq (keyseq, rl_delete);
}

void
prompter_set_kill (const char *keyseq)
{
  rl_bind_keyseq (keyseq, rl_kill_full_line);
}

char *
prompter_get_value (const char *name)
{
  char *prompt = NULL;
  char *val;

  if (name)
    {
      prompt = mu_alloc (strlen (name) + 3);
      strcpy (prompt, name);
      strcat (prompt, ": ");
    }
  val = readline (prompt);
  free (prompt);
  return val;
}

char *
prompter_get_line ()
{
  return doteof_filter (readline (NULL));
}


