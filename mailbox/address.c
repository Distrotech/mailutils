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

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include <mailutils/parse822.h>
#include <misc.h>
#include <address0.h>

/* Get email address from rfc822 address.  */
int
address_create (address_t *a, const char *s)
{
  /* 'paddress' must exist, and can't already have been initialized
   */
  int status;
  const char *e;
  const char *save;
  char *fb;

  if (!a)
    return EINVAL;

  *a = NULL;
  save = s;
  e = &s[strlen (s)];
  fb = calloc (1, 1);

  /* We need to unfold the string. Do the same thing as parse822_field_body()
     but we have to be more flexible in allowing bare '\n' as CRLF.  */
  for (;;)
    {
      const char *eol = s;
      size_t len = strlen (fb);
      while (eol != e)
	{
	  /* if (eol[0] == '\r' && (eol+1) != e && eol[1] == '\n') */
	  if (*eol == '\n')
	    break;
	  ++eol;
	}

      fb = realloc (fb, len + (eol - s) + 1);
      memcpy (fb + len , s, eol - s);
      fb[len + (eol - s)] = '\0';

      s = eol;
      s += 2;

      if (s == e)
	break; /* no more, so we're done */

      /* check if next line is a continuation line */
      if (*s != ' ' && *s != '\t')
	break;
    }

  status = parse822_address_list (a, (char*) fb);
  free (fb);
  if (status == 0)
    {
      /* And address-list may contain 0 addresses but parse correctly.
       */
      if (!*a)
	return ENOENT;

      (*a)->addr = strdup (save);
      if (!(*a)->addr)
	{
	  address_destroy (a);
	  return ENOMEM;
        }
    }
  return status;
}

void
address_destroy (address_t *paddress)
{
  if (paddress && *paddress)
    {
      address_t address = *paddress;
      address_t current;
      for (; address; address = current)
	{
	  if (address->addr)
	    free (address->addr);
	  if (address->comments)
	    free (address->comments);
	  if (address->personal)
	    free (address->personal);
	  if (address->email)
	    free (address->email);
	  if (address->local_part)
	    free (address->local_part);
	  if (address->domain)
	    free (address->domain);
	  if (address->route)
	    free (address->route);
	  current = address->next;
	  free (address);
	}
      *paddress = NULL;
    }
}

int
address_get_personal (address_t addr, size_t no, char *buf, size_t len,
		      size_t *n)
{
  size_t i, j;
  int status = ENOENT;
  if (addr == NULL)
    return EINVAL;
  for (i = 0, j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  i = _cpystr (buf, addr->personal, len);
	  status = 0;
	  break;
	}
    }
  if (n)
    *n = i;
  return status;
}

int
address_get_comments (address_t addr, size_t no, char *buf, size_t len,
		      size_t *n)
{
  size_t i, j;
  int status = ENOENT;
  if (addr == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  i = _cpystr (buf, addr->comments, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

int
address_get_email (address_t addr, size_t no, char *buf, size_t len, size_t *n)
{
  size_t i, j;
  int status = ENOENT;
  if (addr == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  i = _cpystr (buf, addr->email, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

int
address_get_local_part (address_t addr, size_t no, char *buf, size_t len, size_t *n)
{
  size_t i, j;
  int status = ENOENT;
  if (addr == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  i = _cpystr (buf, addr->local_part, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

int
address_get_domain (address_t addr, size_t no, char *buf, size_t len, size_t *n)
{
  size_t i, j;
  int status = ENOENT;
  if (addr == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  i = _cpystr (buf, addr->domain, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

int
address_get_route (address_t addr, size_t no, char *buf, size_t len, size_t *n)
{
  size_t i, j;
  int status = ENOENT;
  if (addr == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  i = _cpystr (buf, addr->route, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

int
address_is_group (address_t addr, size_t no, int* yes)
{
  size_t j;
  int status = ENOENT;
  if(addr == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  status = 0;
	  if(yes)
	    {
	       *yes = 0;
	       if(addr->personal && !addr->local_part && !addr->domain)
	         *yes = 1;
	    }
	  break;
	}
    }
  return status;
}

int
address_to_string (address_t addr, char *buf, size_t len, size_t *n)
{
  size_t i;
  if (addr == NULL)
    return EINVAL;
  if (buf)
    *buf = '\0';
  i = _cpystr (buf, addr->addr, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_count (address_t addr, size_t *pcount)
{
  size_t j;
  for (j = 0; addr; addr = addr->next, j++)
    ;
  if (pcount)
    *pcount = j;
  return 0;
}
