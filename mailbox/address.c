/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include <sys/types.h>

#include <mailutils/errno.h>
#include <mailutils/mutil.h>
#include <mailutils/parse822.h>

#include <address0.h>

/* Get email addresses from rfc822 address.  */
int
address_create (address_t * a, const char *s)
{
  /* 'a' must exist, and can't already have been initialized
   */
  int status;

  if (!a)
    return MU_ERR_OUT_PTR_NULL;

  if (!s)
    return EINVAL;

  *a = NULL;
  status = parse822_address_list (a, s);
  if (status == 0)
    {
      /* And address-list may contain 0 addresses but parse correctly.
       */
      if (!*a)
	return MU_ERR_EMPTY_ADDRESS;

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
address_createv (address_t * a, const char *sv[], size_t len)
{
  int status = 0;
  size_t buflen = 0;
  char *buf = 0;
  size_t i;

  if (!a)
    return MU_ERR_OUT_PTR_NULL;

  if (!sv)
    return EINVAL;

  if (len == (size_t) - 1)
    {
      const char **vp = sv;

      len = 0;

      for (len = 0; *vp; vp++, len++)
	;
    }

  if (len == 0)
    return EINVAL;

  for (i = 0; i < len; i++)
    {
      /* NULL strings are allowed */
      if (sv[i])
	buflen += strlen (sv[i]);
    }

  buflen += (len - 1) * strlen (", ");
  buflen += 1;			/* Termination null.  */

  buf = malloc (buflen);

  if (!buf)
    return ENOMEM;

  for (i = 0, buf[0] = '\0'; i < len; i++)
    {
      if (i != 0)
	strcat (buf, ", ");

      if (sv[i])
	strcat (buf, sv[i]);
    }

  status = address_create (a, buf);

  free (buf);

  return status;
}

void
address_destroy (address_t * paddress)
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
address_concatenate (address_t to, address_t * from)
{
  if (!to || !from || !*from)
    return EINVAL;

  while (to->next)
    to = to->next;

  assert (to && !to->next);

  to->next = *from;
  *from = NULL;

  /* discard the current string cache as it is now inaccurate */
  if (to->addr)
    {
      free (to->addr);
      to->addr = NULL;
    }

  to = to->next;

  /* only the first address must have a cache */
  if (to->addr)
    {
      free (to->addr);
      to->addr = NULL;
    }

  return 0;
}

address_t 
_address_get_nth (address_t addr, size_t no)
{
  int i;
  
  for (i = 1; addr; addr = addr->next, i++)
    if (i == no)
      break;
  return addr;
}

int
address_get_nth (address_t addr, size_t no, address_t *pret)
{
  address_t subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  *pret = address_dup (subaddr);
  return 0;
}

int
address_get_personal (address_t addr, size_t no, char *buf, size_t len,
		      size_t * n)
{
  size_t i;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  i = mu_cpystr (buf, subaddr->personal, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_comments (address_t addr, size_t no, char *buf, size_t len,
		      size_t * n)
{
  size_t i;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;

  i = mu_cpystr (buf, subaddr->comments, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_email (address_t addr, size_t no, char *buf, size_t len,
		   size_t * n)
{
  size_t i;
  address_t subaddr;

  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  i = mu_cpystr (buf, subaddr->email, len);
  if (n)
    *n = i;
  return 0;
}

#define format_char(c) do {\
 if (buflen) \
   {\
      *buf++ = c;\
      buflen--;\
   }\
 else\
   rc++;\
} while(0) 

#define format_string(str) do {\
 if (buflen) \
   {\
      int n = snprintf (buf, buflen, "%s", str);\
      buf += n;\
      buflen -= n;\
   }\
 else\
   rc += strlen (str);\
} while (0)
     
size_t
address_format_string (address_t addr, char *buf, size_t buflen)
{
  int rc = 0;
  int comma = 0;
  
  for (;addr; addr = addr->next)
    {
      if (addr->email)
	{
	  int space = 0;

	  if (comma)
	    format_char (',');
	  
	  if (addr->personal)
	    {
	      format_char ('"');
	      format_string (addr->personal);
	      format_char ('"');
	      space++;
	    }
	  
	  if (addr->comments)
	    {
	      if (space)
		format_char (' ');
	      format_char ('(');
	      format_string (addr->comments);
	      format_char (')');
	      space++;
	    }
	  
	  if (space)
	    format_char (' ');
	  format_char ('<');
	  format_string (addr->email);
	  format_char ('>');
	  comma++;
	}
    }
  format_char (0);
  return rc;
}

int
address_aget_personal (address_t addr, size_t no, char **buf)
{
  int status = 0;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;

  if (subaddr->personal)
    {
      *buf = strdup (subaddr->personal);
      if (!*buf)
	status = ENOMEM;
    }
  else
    *buf = NULL;
  return status;
}

int
address_aget_comments (address_t addr, size_t no, char **buf)
{
  int status = 0;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;

  if (subaddr->comments)
    {
      *buf = strdup (subaddr->comments);
      if (!*buf)
	status = ENOMEM;
    }
  else
    *buf = NULL;
  return status;
}

int
address_aget_email (address_t addr, size_t no, char **buf)
{
  int status = 0;
  address_t subaddr;
  
  if (addr == NULL || buf == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  if (subaddr->email)
    {
      *buf = strdup (subaddr->email);
      if (!*buf)
	status = ENOMEM;
    }
  else
    *buf = NULL;

  return status;
}

int
address_aget_local_part (address_t addr, size_t no, char **buf)
{
  int status = 0;
  address_t subaddr;
  
  if (addr == NULL || buf == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  if (subaddr->local_part)
    {
      *buf = strdup (subaddr->local_part);
      if (!*buf)
	status = ENOMEM;
    }
  else
    *buf = NULL;

  return status;
}

int
address_aget_domain (address_t addr, size_t no, char **buf)
{
  int status = 0;
  address_t subaddr;
  
  if (addr == NULL || buf == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  if (subaddr->domain)
    {
      *buf = strdup (subaddr->domain);
      if (!*buf)
	status = ENOMEM;
    }
  else
    *buf = NULL;

  return status;
}

int
address_get_local_part (address_t addr, size_t no, char *buf, size_t len,
			size_t * n)
{
  size_t i;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;

  i = mu_cpystr (buf, subaddr->local_part, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_domain (address_t addr, size_t no, char *buf, size_t len,
		    size_t * n)
{
  size_t i;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  i = mu_cpystr (buf, subaddr->domain, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_route (address_t addr, size_t no, char *buf, size_t len,
		   size_t * n)
{
  size_t i;
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  i = mu_cpystr (buf, subaddr->route, len);
  if (n)
    *n = i;
  return 0;
}

static int
_address_is_group (address_t addr)
{
  if (addr->personal && !addr->local_part && !addr->domain)
    return 1;
  return 0;
}

static int
_address_is_email (address_t addr)
{
  if (addr->email)
    return 1;
  return 0;
}

static int
_address_is_unix_mailbox (address_t addr)
{
  if (addr->local_part && !addr->email)
    return 1;
  return 0;
}

int
address_is_group (address_t addr, size_t no, int *yes)
{
  address_t subaddr;
  
  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  
  if (yes)
    *yes = _address_is_group (subaddr);
  return 0;
}

int
address_to_string (address_t addr, char *buf, size_t len, size_t * n)
{
  size_t i;
  if (addr == NULL)
    return EINVAL;
  if (buf)
    *buf = '\0';

  if (!addr->addr)
    {
      i = address_format_string (addr, NULL, 0);
      addr->addr = malloc (i + 1);
      if (!addr->addr)
	return ENOMEM;
      address_format_string (addr, addr->addr, i+1);
    }
      
  i = mu_cpystr (buf, addr->addr, len);
  if (n)
    *n = i;
  return 0;
}

int
address_get_count (address_t addr, size_t * pcount)
{
  size_t j;
  for (j = 0; addr; addr = addr->next, j++)
    ;
  if (pcount)
    *pcount = j;
  return 0;
}

int
address_get_group_count (address_t addr, size_t * pcount)
{
  size_t j;
  for (j = 0; addr; addr = addr->next)
    {
      if (_address_is_group (addr))
	j++;
    }
  if (pcount)
    *pcount = j;
  return 0;
}

int
address_get_email_count (address_t addr, size_t * pcount)
{
  size_t j;
  for (j = 0; addr; addr = addr->next)
    {
      if (_address_is_email (addr))
	j++;
    }
  if (pcount)
    *pcount = j;
  return 0;
}

int
address_get_unix_mailbox_count (address_t addr, size_t * pcount)
{
  size_t j;
  for (j = 0; addr; addr = addr->next)
    {
      if (_address_is_unix_mailbox (addr))
	j++;
    }
  if (pcount)
    *pcount = j;
  return 0;
}

int
address_contains_email (address_t addr, const char *email)
{
  for (; addr; addr = addr->next)
    if (strcasecmp (addr->email, email) == 0)
      return 1;
  return 0;
}

address_t
address_dup (address_t src)
{
  address_t dst = calloc (1, sizeof (*dst));

  if (!dst)
    return NULL;

  if (src->comments)
    dst->comments = strdup (src->comments);
  if (src->personal)
    dst->personal = strdup (src->personal);
  if (src->email)
    dst->email = strdup (src->email);
  if (src->local_part)
    dst->local_part = strdup (src->local_part);
  if (src->domain)
    dst->domain = strdup (src->domain);
  if (src->route)
    dst->route = strdup (src->route);

  return dst;
}
  
int
address_union (address_t *a, address_t b)
{
  address_t last = NULL;
    
  if (!a || !b)
    return EINVAL;

  if (!*a)
    {
      *a = address_dup (b);
      if (!*a)
	return ENOMEM;
      last = *a;
      b = b->next;
    }
  else
    {
      if ((*a)->addr)
	{
	  free ((*a)->addr);
	  (*a)->addr = NULL;
	}
      for (last = *a; last->next; last = last->next)
	;
    }

  for (; b; b = b->next)
    if (!address_contains_email (*a, b->email))
      {
	address_t next = address_dup (b);
	if (!next)
	  return ENOMEM;
	last->next = next;
	last = next;
      }
  return 0;
}
  
