/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005,
   2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>

#include <mailutils/mutil.h>
#include <mailutils/errno.h>
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
mu_ticket_create (mu_ticket_t *pticket, void *owner)
{
  mu_ticket_t ticket;
  if (pticket == NULL)
    return MU_ERR_OUT_PTR_NULL;
  ticket = calloc (1, sizeof (*ticket));
  if (ticket == NULL)
    return ENOMEM;
  ticket->owner = owner;
  *pticket = ticket;
  return 0;
}

void
mu_ticket_destroy (mu_ticket_t *pticket, void *owner)
{
  if (pticket && *pticket)
    {
      mu_ticket_t ticket = *pticket;
      if (ticket->owner == owner)
	{
	  if (ticket->_destroy)
	    ticket->_destroy (ticket);
	  if (ticket->challenge)
	    free (ticket->challenge);
	  free (ticket);
	}
      *pticket = NULL;
    }
}

int
mu_ticket_set_destroy (mu_ticket_t ticket, void (*_destroy) (mu_ticket_t),
		    void *owner)
{
  if (ticket == NULL)
    return EINVAL;
  if (ticket->owner != owner)
    return EACCES;
  ticket->_destroy = _destroy;
  return 0;
}

void *
mu_ticket_get_owner (mu_ticket_t ticket)
{
  return (ticket) ? ticket->owner : NULL;
}

int
mu_ticket_set_pop (mu_ticket_t ticket,
		int (*_pop) (mu_ticket_t, mu_url_t, const char *, char **),
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
mu_ticket_pop (mu_ticket_t ticket, mu_url_t url, const char *challenge, char **parg)
{
  int rc = -1;
  
  if (ticket == NULL)
    return EINVAL;
  if (parg == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (ticket->_pop)
    rc = ticket->_pop (ticket, url, challenge, parg);
  if (rc != 0 && isatty (fileno (stdin)))
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
      rc = 0;
    }
  return rc;
}

int
mu_ticket_get_data (mu_ticket_t ticket, void **data)
{
  if (ticket == NULL)
    return EINVAL;
  if (data == NULL)
    return MU_ERR_OUT_PTR_NULL;
  *data = ticket->data;
  return 0;
}

int
mu_ticket_set_data (mu_ticket_t ticket, void *data, void *owner)
{
  if (ticket == NULL)
    return EINVAL;
  if (ticket->owner != owner)
    return EACCES;
  ticket->data = data;
  return 0;
}
