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
#include <mailutils/monitor.h>
#include <mailutils/sys/ticket.h>

struct _prompt_ticket
{
  struct _ticket base;
  int ref;
  monitor_t lock;
};

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

static int
_prompt_add_ref (ticket_t ticket)
{
  int status;
  struct _prompt_ticket *prompt = (struct _prompt_ticket *)ticket;
  monitor_lock (prompt->lock);
  status = ++prompt->ref;
  monitor_unlock (prompt->lock);
  return status;
}

static int
_prompt_destroy (ticket_t ticket)
{
  struct _prompt_ticket *prompt = (struct _prompt_ticket *)ticket;
  monitor_destroy (prompt->lock);
  free (prompt);
  return 0;
}

static int
_prompt_release (ticket_t ticket)
{
  int status;
  struct _prompt_ticket *prompt = (struct _prompt_ticket *)ticket;
  monitor_lock (prompt->lock);
  status = --prompt->ref;
  if (status <= 0)
    {
      monitor_unlock (prompt->lock);
      _prompt_destroy (ticket);
      return 0;
    }
  monitor_unlock (prompt->lock);
  return status;
}

static int
_prompt_pop (ticket_t ticket, const char *challenge, char **parg)
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

static struct _ticket_vtable _prompt_vtable =
{
  _prompt_add_ref,
  _prompt_release,
  _prompt_destroy,

  _prompt_pop,
};

int
ticket_prompt_create (ticket_t *pticket)
{
  struct _prompt_ticket *prompt;

  if (pticket == NULL)
    return MU_ERROR_INVALID_PARAMETER;

  prompt = calloc (1, sizeof *prompt);
  if (prompt == NULL)
    return MU_ERROR_NO_MEMORY;

  prompt->base.vtable = &_prompt_vtable;
  prompt->ref = 1;
  monitor_create (&(prompt->lock));
  *pticket = &prompt->base;
  return 0;
}

