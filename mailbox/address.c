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
#include <mailutils/mutil.h>
#include <address0.h>

/* Get email addresses from rfc822 address.  */
int
address_create (address_t *a, const char *s)
{
  /* 'paddress' must exist, and can't already have been initialized
   */
  int status;

  if (!a)
    return EINVAL;

  *a = NULL;
  status = parse822_address_list (a, s);
  if (status == 0)
    {
      /* And address-list may contain 0 addresses but parse correctly.
       */
      if (!*a)
	return ENOENT;

      (*a)->addr = strdup (s);
      if (!(*a)->addr)
	{
	  address_destroy (a);
	  return ENOMEM;
        }
    }
  return status;
}

/* Get email addresses from array of rfc822 addresses. */
int
address_createv (address_t *a, const char *sv[], size_t len)
{
  int status = 0;
  size_t buflen = 0;
  char* buf = 0;
  size_t i;

  if (!sv)
    return EINVAL;

  if (len == (size_t)-1)
    {
      const char** vp = sv;

      len = 0;

      for (len = 0; *vp; vp++, len++)
	;
    }

  if (len == 0)
    return EINVAL;

  for (i = 0; i < len; i++)
    buflen += strlen(sv[i]);

  buflen += (len - 1) * strlen (", ");
  buflen += 1; /* Termination null.  */

  buf = malloc (buflen);

  if (!buf)
    return ENOMEM;

  for (i = 0, buf[0] = '\0'; i < len; i++)
    {
      if(i != 0)
	strcat (buf, ", ");

      strcat (buf, sv[i]);
    }

  status = address_create (a, buf);

  free (buf);

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
	  i = mu_cpystr (buf, addr->personal, len);
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
	  i = mu_cpystr (buf, addr->comments, len);
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
	  i = mu_cpystr (buf, addr->email, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

int
address_aget_email (address_t addr, size_t no, char **buf)
{
  size_t j;
  int status = ENOENT;
  if (addr == NULL || buf == NULL)
    return EINVAL;
  for (j = 1; addr; addr = addr->next, j++)
    {
      if (j == no)
	{
	  status = 0;
	  *buf = NULL;
	  if (addr->email)
	    {
	      *buf = strdup (addr->email);
	      if (!*buf)
		status = ENOMEM;
	    }
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
	  i = mu_cpystr (buf, addr->local_part, len);
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
	  i = mu_cpystr (buf, addr->domain, len);
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
	  i = mu_cpystr (buf, addr->route, len);
	  if (n)
	    *n = i;
	  status = 0;
	  break;
	}
    }
  return status;
}

static int
_address_is_group (address_t addr)
{
  if (addr->personal && !addr->local_part && !addr->domain)
    return 1;
  return 0;
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
	       *yes = _address_is_group(addr);
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
  i = mu_cpystr (buf, addr->addr, len);
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

int
address_get_email_count (address_t addr, size_t *pcount)
{
  size_t j;
  for (j = 0; addr; addr = addr->next)
  {
    if(!_address_is_group(addr))
      j++;
  }
  if (pcount)
    *pcount = j;
  return 0;
}

