/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2005-2007, 2009-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

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
#include <mailutils/util.h>
#include <mailutils/parse822.h>
#include <mailutils/address.h>
#include <mailutils/cstr.h>
#include <mailutils/stream.h>

int
mu_address_create_null (mu_address_t *pa)
{
  mu_address_t a = calloc (1, sizeof (*a));
  if (!a)
    return ENOMEM;
  *pa = a;
  return 0;
}

/* Get email addresses from rfc822 address.  */
int
mu_address_create_hint (mu_address_t *a, const char *s, mu_address_t hint,
			int hflags)
{
  /* 'a' must exist, and can't already have been initialized
   */
  int status;

  if (!a)
    return MU_ERR_OUT_PTR_NULL;

  if (!s)
    return EINVAL;

  *a = NULL;
  status = mu_parse822_address_list (a, s, hint, hflags);
  if (status == 0)
    {
      /* And address-list may contain 0 addresses but parse correctly.
       */
      if (!*a)
	return MU_ERR_EMPTY_ADDRESS;

      (*a)->printable = strdup (s);
      if (!(*a)->printable)
	{
	  mu_address_destroy (a);
	  return ENOMEM;
	}
    }
  return status;
}

int
mu_address_create (mu_address_t *a, const char *s)
{
  struct mu_address hint;
  const char *d;
  mu_get_user_email_domain (&d);
  hint.domain = (char*) d;
  return mu_address_create_hint (a, s, &hint, MU_ADDR_HINT_DOMAIN);
}

/* Get email addresses from array of rfc822 addresses.
   FIXME: No hints? */
int
mu_address_createv (mu_address_t *a, const char *sv[], size_t len)
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

  status = mu_address_create (a, buf);

  free (buf);

  return status;
}

static void
_address_free (mu_address_t address)
{
  free (address->printable);
  free (address->comments);
  free (address->personal);
  free (address->email);
  free (address->local_part);
  free (address->domain);
  free (address->route);
}

void
mu_address_destroy (mu_address_t *paddress)
{
  if (paddress && *paddress)
    {
      mu_address_t address = *paddress;
      mu_address_t current;
      for (; address; address = current)
	{
	  current = address->next;
	  _address_free (address);
	  free (address);
	}
      *paddress = NULL;
    }
}

int
mu_address_concatenate (mu_address_t to, mu_address_t *from)
{
  if (!to || !from || !*from)
    return EINVAL;

  while (to->next)
    to = to->next;

  assert (to && !to->next);

  to->next = *from;
  *from = NULL;

  /* discard the current string cache as it is now inaccurate */
  if (to->printable)
    {
      free (to->printable);
      to->printable = NULL;
    }

  to = to->next;

  /* only the first address must have a cache */
  if (to->printable)
    {
      free (to->printable);
      to->printable = NULL;
    }

  return 0;
}

mu_address_t
_address_get_nth (mu_address_t addr, size_t no)
{
  int i;

  for (i = 1; addr; addr = addr->next, i++)
    if (i == no)
      break;
  return addr;
}

int
mu_address_get_nth (mu_address_t addr, size_t no, mu_address_t *pret)
{
  mu_address_t subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;
  *pret = mu_address_dup (subaddr);
  return 0;
}


/* General accessors: */
#define AC4(a,b,c,d) a ## b ## c ## d
#define ACCESSOR(action,field) AC4(mu_address_,action,_,field)

#define DECL_SET(field)						        \
int									\
ACCESSOR(set, field) (mu_address_t addr, size_t no, const char *buf)	\
{									\
  char *s;								\
  mu_address_t subaddr;							\
									\
  if (addr == NULL)							\
    return EINVAL;							\
									\
  subaddr = _address_get_nth (addr, no);				\
  if (!subaddr)								\
    return MU_ERR_NOENT;						\
									\
  if (buf)						                \
    {                                                                   \
       s = strdup (buf);						\
       if (!s)								\
	 return errno;							\
    }							                \
  else									\
    s = (char *) buf;							\
									\
  free (subaddr->field);						\
  subaddr->field = s;							\
									\
  return 0;								\
}

#define DECL_SET_EI(field)					        \
int									\
ACCESSOR(set, field) (mu_address_t addr, size_t no, const char *buf)	\
{									\
  char *s;								\
  mu_address_t subaddr;							\
									\
  if (addr == NULL)							\
    return EINVAL;							\
									\
  subaddr = _address_get_nth (addr, no);				\
  if (!subaddr)								\
    return MU_ERR_NOENT;						\
									\
  if (buf)						                \
    {                                                                   \
       s = strdup (buf);						\
       if (!s)								\
	 return errno;							\
    }							                \
  else									\
    s = (char *) buf;							\
									\
  free (subaddr->field);						\
  subaddr->field = s;							\
									\
  free (subaddr->email);						\
  subaddr->email = NULL;						\
									\
  return 0;								\
}

#define DECL_SGET(field)						\
int									\
ACCESSOR(sget,field) (mu_address_t addr, size_t no, char const **sptr)	\
{									\
  mu_address_t subaddr;							\
									\
  if (addr == NULL)							\
    return EINVAL;							\
									\
  subaddr = _address_get_nth (addr, no);				\
  if (!subaddr)								\
    return MU_ERR_NOENT;						\
  *sptr = subaddr->field;						\
  return 0;								\
}

#define DECL_GET(field)							  \
int									  \
ACCESSOR(get,field) (mu_address_t addr, size_t no, char *buf, size_t len, \
		     size_t *n)						  \
{									  \
  size_t i;								  \
  const char *str;							  \
  int status = ACCESSOR(sget, field) (addr, no, &str);			  \
									  \
  if (status)								  \
    return status;							  \
									  \
  i = mu_cpystr (buf, str, len);					  \
  if (n)								  \
    *n = i;								  \
  return 0;								  \
}

#define DECL_AGET(field)						\
int									\
ACCESSOR(aget, field) (mu_address_t addr, size_t no, char **buf)	\
{									\
  const char *str;							\
  int status = ACCESSOR(sget, field) (addr, no, &str);			\
									\
  if (status)								\
    return status;							\
									\
  if (str)								\
    {									\
      *buf = strdup (str);						\
      if (!*buf)							\
	status = ENOMEM;						\
    }									\
  else									\
    *buf = NULL;							\
  return status;							\
}

#define DECL_ACCESSORS(field)			\
DECL_SET(field)					\
DECL_SGET(field)				\
DECL_GET(field)					\
DECL_AGET(field)

#define DECL_ACCESSORS_EI(field)		\
DECL_SET_EI(field)				\
DECL_SGET(field)			        \
DECL_GET(field)					\
DECL_AGET(field)



/* Personal part */
DECL_ACCESSORS(personal)
/* Comments */
DECL_ACCESSORS(comments)
/* Local part */
DECL_ACCESSORS_EI(local_part)
/* Domain */
DECL_ACCESSORS_EI(domain)
/* Route */
DECL_ACCESSORS(route)

/* Email */
int
mu_address_set_email (mu_address_t addr, size_t no, const char *buf)
{
  char *s;
  mu_address_t subaddr;

  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;

  if (buf)
    {
      s = strdup (buf);
      if (!s)
	return errno;
    }
  else
    s = (char *) buf;

  free (subaddr->email);
  subaddr->email = s;

  free (subaddr->local_part);
  free (subaddr->domain);
  if (s)
    {
      char *p = strchr (subaddr->email, '@');
      if (p)
	{
	  size_t len = p - subaddr->email;
	  subaddr->local_part = malloc (len + 1);
	  if (subaddr->local_part)
	    {
	      memcpy (subaddr->local_part, p, len);
	      subaddr->local_part[len] = 0;
	    }
	  subaddr->domain = strdup (p + 1);
	}
    }
  else
    {
      subaddr->local_part = NULL;
      subaddr->domain = NULL;
    }

  return 0;
}

int
mu_validate_email (mu_address_t subaddr)
{
  if (!subaddr->email)
    {
      if (subaddr->local_part)
	{
	  const char *domain;

	  if (subaddr->domain)
	    domain = subaddr->domain;
	  else
	    mu_get_user_email_domain (&domain);
	  if (domain)
	    {
	      char *p;
	      subaddr->email = malloc (strlen (subaddr->local_part) +
				       strlen (domain) + 2);
	      if (!subaddr->email)
		return ENOMEM;
	      p = mu_stpcpy (subaddr->email, subaddr->local_part);
	      *p++ = '@';
	      mu_stpcpy (p, (char*) domain);
	    }
	}
    }
  return 0;
}

int
mu_address_sget_email (mu_address_t addr, size_t no, char const **sptr)
{
  mu_address_t subaddr;

  if (addr == NULL)
    return EINVAL;

  subaddr = _address_get_nth (addr, no);
  if (!subaddr)
    return MU_ERR_NOENT;

  mu_validate_email (subaddr);
  *sptr = subaddr->email;
  return 0;
}

DECL_GET(email)
DECL_AGET(email)

size_t
mu_address_format_string (mu_address_t addr, char *buf, size_t buflen)
{
  mu_stream_t str;
  int rc;
  
  if (!buf)
    rc = mu_nullstream_create (&str, MU_STREAM_WRITE);
  else
    rc = mu_fixed_memory_stream_create (&str, buf, buflen, MU_STREAM_WRITE);
  if (rc == 0)
    {
      size_t size;
      
      mu_stream_stat_buffer statbuf;
      mu_stream_set_stat (str, MU_STREAM_STAT_MASK (MU_STREAM_STAT_OUT),
			  statbuf);
      rc = mu_stream_format_address (str, addr);
      mu_stream_destroy (&str);
      if (rc)
	return 0;
      size = statbuf[MU_STREAM_STAT_OUT];
      if (buf)
	{
	  if (size + 1 >= buflen)
	    size = buflen - 1;
	  buf[size] = 0;
	}
      return size;
    }
  return 0;
}

static int
_address_is_group (mu_address_t addr)
{
  if (addr->personal && !addr->local_part && !addr->domain)
    return 1;
  return 0;
}

static int
_address_is_email (mu_address_t addr)
{
  if (addr->email)
    return 1;
  return 0;
}

static int
_address_is_unix_mailbox (mu_address_t addr)
{
  if (addr->local_part && !addr->email)
    return 1;
  return 0;
}

int
mu_address_is_group (mu_address_t addr, size_t no, int *yes)
{
  mu_address_t subaddr;

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
mu_address_sget_printable (mu_address_t addr, const char **sptr)
{
  if (addr == NULL)
    return EINVAL;
  if (!sptr)
    return MU_ERR_OUT_PTR_NULL;
  if (!addr->printable)
    {
      mu_stream_t str;
      int rc;
      
      rc = mu_memory_stream_create (&str, MU_STREAM_RDWR);
      if (rc)
	return rc;
      rc = mu_stream_format_address (str, addr);
      if (rc == 0)
	{
	  mu_off_t size;
	  mu_stream_size (str, &size);
	  addr->printable = malloc (size + 1);
	  if (!addr->printable)
	    rc = ENOMEM;
	  else
	    {
	      mu_stream_seek (str, 0, MU_SEEK_SET, NULL);
	      rc = mu_stream_read (str, addr->printable, size, NULL);
	      addr->printable[size] = 0;
	    }
	}
      mu_stream_destroy (&str);
      if (rc)
	return rc;
    }
  *sptr = addr->printable;
  return 0;
}

int
mu_address_aget_printable (mu_address_t addr, char **presult)
{
  int rc;
  const char *s;

  if (addr == NULL)
    return EINVAL;
  if (!presult)
    return MU_ERR_OUT_PTR_NULL;
  rc = mu_address_sget_printable (addr, &s);
  if (rc == 0)
    {
      char *result = strdup (s);
      if (result)
	*presult = result;
      else
	rc = ENOMEM;
    }
  return rc;
}

int
mu_address_get_printable (mu_address_t addr, char *buf, size_t len, size_t *n)
{
  const char *s;
  int rc;
  
  rc = mu_address_sget_printable (addr, &s);
  if (rc == 0)
    {
      size_t i;
      i = mu_cpystr (buf, addr->printable, len);
      if (n)
	*n = i;
    }
  return rc;
}

int
mu_address_to_string (mu_address_t addr, char *buf, size_t len, size_t *n)
{
  return mu_address_get_printable (addr, buf, len, n);
}

int
mu_address_get_count (mu_address_t addr, size_t *pcount)
{
  size_t i, count = 0;
  for (i = 0; addr; addr = addr->next, i++)
    {
      mu_validate_email (addr);
      if (addr->email)
	++count;
    }
  if (pcount)
    *pcount = count;
  return 0;
}

int
mu_address_get_group_count (mu_address_t addr, size_t *pcount)
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
mu_address_get_email_count (mu_address_t addr, size_t *pcount)
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
mu_address_get_unix_mailbox_count (mu_address_t addr, size_t *pcount)
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
mu_address_contains_email (mu_address_t addr, const char *email)
{
  for (; addr; addr = addr->next)
    {
      mu_validate_email (addr);
      if (!addr->email)
	break;
      if (mu_c_strcasecmp (addr->email, email) == 0)
	return 1;
    }
  return 0;
}

static int
mu_list_copy (mu_address_t dst, mu_address_t src)
{
  /* FIXME: How about:
    if (src->printable)
      dst->printable = strdup (src->printable);
    ?
  */
  if (src->comments && !(dst->comments = strdup (src->comments)))
    return ENOMEM;
  if (src->personal && !(dst->personal = strdup (src->personal)))
    return ENOMEM;
  if (src->email && !(dst->email = strdup (src->email)))
    return ENOMEM;
  if (src->local_part && !(dst->local_part = strdup (src->local_part)))
    return ENOMEM;
  if (src->domain && !(dst->domain = strdup (src->domain)))
    return ENOMEM;
  if (src->route && !(dst->route = strdup (src->route)))
    return ENOMEM;
  return 0;
}  

mu_address_t
mu_address_dup (mu_address_t src)
{
  mu_address_t dst = calloc (1, sizeof (*dst));

  if (!dst)
    return NULL;
  if (mu_list_copy (dst, src))
    mu_address_destroy (&dst);
  return dst;
}

int
mu_address_union (mu_address_t *a, mu_address_t b)
{
  mu_address_t last = NULL;

  if (!a || !b)
    return EINVAL;

  if (!*a)
    {
      *a = mu_address_dup (b);
      if (!*a)
	return ENOMEM;
      last = *a;
      b = b->next;
    }
  else
    {
      if ((*a)->printable)
	{
	  free ((*a)->printable);
	  (*a)->printable = NULL;
	}
      for (last = *a; last->next; last = last->next)
	;
    }

  for (; b; b = b->next)
    if (!mu_address_contains_email (*a, b->email))
      {
	if (last->email)
	  {
	    mu_address_t next = mu_address_dup (b);
	    if (!next)
	      return ENOMEM;
	    last->next = next;
	    last = next;
	  }
	else
	  {
	    int rc = mu_list_copy (last, b);
	    if (rc)
	      {
		_address_free (last);
		memset (last, 0, sizeof (last));
		return rc;
	      }
	  }
      }
  return 0;
}
