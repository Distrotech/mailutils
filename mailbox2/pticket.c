/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>

#include <mailutils/error.h>
#include <mailutils/refcount.h>
#include <mailutils/sys/pticket.h>

static void
echo_off(struct termios *stored_settings)
{
  struct termios new_settings;
  tcgetattr (0, stored_settings);
  new_settings = *stored_settings;
  new_settings.c_lflag &= (~ECHO);
  tcsetattr (0, TCSANOW, &new_settings);
}

static void
echo_on(struct termios *stored_settings)
{
  tcsetattr (0, TCSANOW, stored_settings);
}

int
_ticket_prompt_ref (ticket_t ticket)
{
  struct _ticket_prompt *prompt = (struct _ticket_prompt *)ticket;
  return mu_refcount_inc (prompt->refcount);
}

void
_ticket_prompt_destroy (ticket_t *pticket)
{
  struct _ticket_prompt *prompt = (struct _ticket_prompt *)*pticket;
  if (mu_refcount_dec (prompt->refcount) == 0)
    {
      mu_refcount_destroy (&prompt->refcount);
      free (prompt);
    }
}

int
_ticket_prompt_pop (ticket_t ticket, const char *challenge, char **parg)
{
  char arg[256];
  struct termios stored_settings;
  int echo = 1;
  (void)ticket;

  /* Being smart if we see "Passwd" and turning off echo.  */
  if (strstr (challenge, "ass") != NULL
      || strstr (challenge, "ASS") != NULL)
    echo = 0;
  fprintf (stdout, "%s", challenge);
  fflush (stdout);
  if (!echo)
    echo_off (&stored_settings);
  fgets (arg, sizeof (arg), stdin);
  if (!echo)
    {
      echo_on (&stored_settings);
      fputc ('\n', stdout);
      fflush (stdout);
    }
  arg [strlen (arg) - 1] = '\0'; /* nuke the trailing line.  */
  *parg = strdup (arg);
  return 0;
}

static struct _ticket_vtable _ticket_prompt_vtable =
{
  _ticket_prompt_ref,
  _ticket_prompt_destroy,

  _ticket_prompt_pop,
};

int
_ticket_prompt_ctor (struct _ticket_prompt *prompt)
{
  mu_refcount_create (&prompt->refcount);
  if (prompt->refcount == NULL)
    return MU_ERROR_NO_MEMORY;
  prompt->base.vtable = &_ticket_prompt_vtable;
  return 0;
}

void
_ticket_prompt_dtor (struct _ticket_prompt *prompt)
{
  mu_refcount_destroy (&prompt->refcount);
}

int
ticket_prompt_create (ticket_t *pticket)
{
  struct _ticket_prompt *prompt;
  int status;

  if (pticket == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  prompt = calloc (1, sizeof *prompt);
  if (prompt == NULL)
    return MU_ERROR_NO_MEMORY;

  status = _ticket_prompt_ctor (prompt);
  if (status != 0)
    {
      free (prompt);
      return status;
    }
  *pticket = &prompt->base;
  return 0;
}
