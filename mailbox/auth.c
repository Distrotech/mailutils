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

#include <mailutils/mailbox.h>
#include <mailutils/mutil.h>
#include <auth0.h>

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
ticket_create (ticket_t *pticket, void *owner)
{
  ticket_t ticket;
  if (pticket == NULL)
    return EINVAL;
  ticket = calloc (1, sizeof (*ticket));
  if (ticket == NULL)
    return ENOMEM;
  ticket->owner = owner;
  *pticket = ticket;
  return 0;
}

void
ticket_destroy (ticket_t *pticket, void *owner)
{
  if (pticket && *pticket)
    {
      ticket_t ticket = *pticket;
      if (ticket->owner == owner)
	{
	  if (ticket->type)
	    free (ticket->type);
	  free (ticket);
	}
      *pticket = NULL;
    }
}

void *
ticket_get_owner (ticket_t ticket)
{
  return (ticket) ? ticket->owner : NULL;
}

int
ticket_set_pop (ticket_t ticket,
		int (*_pop) __P ((ticket_t, const char *, char **)),
		void *owner)
{
  if (ticket == NULL)
    return EINVAL;
  if (ticket->owner != owner)
    return EACCES;
  ticket->_pop = _pop;
  return 0;
}

int
ticket_pop (ticket_t ticket, const char *challenge, char **parg)
{
  if (ticket == NULL || parg == NULL)
    return EINVAL;
  if (ticket->_pop)
    return ticket->_pop (ticket, challenge, parg);
  else
    {
      char arg[256];
      struct termios stored_settings;
      int echo = 1;

      /* Being smart if we see "Passwd" and turning off echo.  */
      if (strstr (challenge, "ass") != NULL
	  || strstr (challenge, "ASS") != NULL)
	echo = 0;
      printf ("%s", challenge);
      fflush (stdout);
      if (!echo)
	echo_off (&stored_settings);
      fgets (arg, sizeof (arg), stdin);
      if (!echo)
	{
	  echo_on (&stored_settings);
	  putchar ('\n');
	  fflush (stdout);
	}
      arg [strlen (arg) - 1] = '\0'; /* nuke the trailing line.  */
      *parg = strdup (arg);
    }
  return 0;
}

int
ticket_get_type (ticket_t ticket, char *type, size_t len, size_t *pwriten)
{
  size_t n;
  if (ticket == NULL || type == NULL)
    return EINVAL;
  n = util_cpystr (type, ticket->type, len);
  if (pwriten)
    *pwriten = n;
  return 0;
}

int
ticket_set_type (ticket_t ticket, char *type)
{
  if (ticket == NULL)
    return EINVAL;
  if (ticket->type)
    free (ticket->type);
  ticket->type = strdup ((type) ? type : "");
  return 0;
}

int
authority_create (authority_t *pauthority, ticket_t ticket, void *owner)
{
  authority_t authority;
  if (pauthority == NULL)
    return EINVAL;
  authority = calloc (1, sizeof (*authority));
  if (authority == NULL)
    return ENOMEM;
  authority->ticket = ticket;
  authority->owner = owner;
  *pauthority = authority;
  return 0;
}

void
authority_destroy (authority_t *pauthority, void *owner)
{
  if (pauthority && *pauthority)
    {
      authority_t authority = *pauthority;
      if (authority->owner == owner)
	{
	  ticket_destroy (&(authority->ticket), authority);
	  free (authority);
	}
      *pauthority = NULL;
    }
}

void *
authority_get_owner (authority_t authority)
{
  return (authority) ? authority->owner : NULL;
}

int
authority_set_ticket (authority_t authority, ticket_t ticket)
{
  if (authority == NULL)
    return EINVAL;
  if (authority->ticket)
    ticket_destroy (&(authority->ticket), authority);
  authority->ticket = ticket;
  return 0;
}

int
authority_get_ticket (authority_t authority, ticket_t *pticket)
{
  if (authority == NULL || pticket == NULL)
    return EINVAL;
  if (authority->ticket == NULL)
    {
      int status = ticket_create (&(authority->ticket), authority);
      if (status != 0)
	return status;
    }
  *pticket = authority->ticket;
  return 0;
}


int
authority_authenticate (authority_t authority)
{
  if (authority && authority->_authenticate)
    {
      return authority->_authenticate (authority);
    }
  return EINVAL;
}

int
authority_set_authenticate (authority_t authority,
			    int (*_authenticate) __P ((authority_t)),
			    void *owner)
{
  if (authority == NULL)
    return EINVAL;

  if (authority->owner != owner)
    return EACCES;
  authority->_authenticate = _authenticate;
  return 0;
}
