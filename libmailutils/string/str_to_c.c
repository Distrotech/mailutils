/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2016 Free Software Foundation, Inc.

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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <mailutils/cidr.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>
#include <mailutils/util.h>
#include <mailutils/alloc.h>
#include <mailutils/cctype.h>
#include <mailutils/io.h>
#include <mailutils/nls.h>

typedef int (*str_to_c_t) (void *tgt, char const *string, char **errmsg);

static int
to_string (void *tgt, char const *string, char **errmsg)
{
  char **cptr = tgt;
  if (string)
    {
      *cptr = mu_strdup (string);
      if (!*cptr)
	return errno;
    }
  else
    *cptr = NULL;
  return 0;
}

#define STR_TO_FUN to_short
#define STR_TO_TYPE signed short
#define STR_TO_MIN SHRT_MIN
#define STR_TO_MAX SHRT_MAX
#include "to_sn.c"

#define STR_TO_FUN to_ushort
#define STR_TO_TYPE unsigned short
#define STR_TO_MAX USHRT_MAX
#include "to_un.c"

#define STR_TO_FUN to_int
#define STR_TO_TYPE signed int
#define STR_TO_MIN INT_MIN
#define STR_TO_MAX INT_MAX
#include "to_sn.c"

#define STR_TO_FUN to_uint
#define STR_TO_TYPE unsigned int
#define STR_TO_MAX UINT_MAX
#include "to_un.c"

#define STR_TO_FUN to_long
#define STR_TO_TYPE signed long
#define STR_TO_MIN LONG_MIN
#define STR_TO_MAX LONG_MAX
#include "to_sn.c"

#define STR_TO_FUN to_ulong
#define STR_TO_TYPE unsigned long
#define STR_TO_MAX ULONG_MAX
#include "to_un.c"

#define STR_TO_FUN to_size_t
#define STR_TO_TYPE size_t
#define STR_TO_MAX ((size_t)-1)
#include "to_un.c"

static int
time_multiplier (const char *str, unsigned *m, unsigned *plen)
{
  static struct timetab
  {
    char *name;
    unsigned mul;
  } tab[] = {
    { "seconds", 1 },
    { "minutes", 60 },
    { "hours", 60*60 },
    { "days",  24*60*60 },
    { "weeks", 7*24*60*60 },
    { "months", 31*7*24*60*60 },
    { NULL }
  };
  struct timetab *p;
  int slen;

  for (slen = 0; str[slen]; slen++)
    if (mu_isspace (str[slen]))
      break;

  for (p = tab; p->name; p++)
    {
      if (p->name[0] == mu_tolower (str[0]))
	{
	  int nlen = strlen (p->name);

	  if (nlen > slen)
	    nlen = slen;
			
	  if (strncasecmp (p->name, str, nlen) == 0) {
	    *m = p->mul;
	    if (plen)
	      *plen = nlen;
	    return 0;
	  }
	}
    }
  return 1;
}

static int
to_time_t (void *tgt, char const *string, char **errmsg)
{
  time_t *ptr = tgt;
  int rc = 0;
  time_t interval = 0;

  while (*string)
    {
      char *p;
      unsigned long n;
      unsigned mul, len;

      while (*string && mu_isspace (*string))
	string++;

      if (!mu_isdigit (*string) && time_multiplier (string, &mul, &len) == 0)
	{
	  n = 1;
	  string += len;
	}
      else
	{
	  n = strtoul (string, &p, 10);
	  if (*p && !mu_isspace (*p))
	    {
	      string = p;
	      rc = 1;
	      break;
	    }
	  
	  while (*p && mu_isspace (*p))
	    p++;

	  string = p;
	  if (*string)
	    {
	      if ((rc = time_multiplier (string, &mul, &len)))
		break;
	      string += len;
	    }
	  else
	    mul = 1;
	}
      interval += n * mul;
    }

  if (rc)
    {
      if (errmsg)
	mu_asprintf (errmsg, _("invalid time specification near %s"), string);
      return EINVAL;
    }
  
  *ptr = interval;
  return 0;
}
  
static int
to_bool (void *tgt, char const *string, char **errmsg)
{
  int *ptr = tgt;
  
  if (strcmp (string, "yes") == 0
      || strcmp (string, "on") == 0
      || strcmp (string, "t") == 0
      || strcmp (string, "true") == 0
      || strcmp (string, "1") == 0)
    *ptr = 1;
  else if (strcmp (string, "no") == 0
	   || strcmp (string, "off") == 0
	   || strcmp (string, "nil") == 0
	   || strcmp (string, "false") == 0
	   || strcmp (string, "0") == 0)
    *ptr = 0;
  else
    return EINVAL;
  
  return 0;
}

#if 0
static int
to_ipv4 (void *tgt, char const *string, char **errmsg)
{
  struct in_addr *ptr = tgt;
  struct in_addr addr;
  
  if (inet_aton (string, &addr) == 0)
    {
      mu_diag_at_locus (MU_LOG_ERROR, &mu_cfg_locus, _("not an IPv4"));
      mu_cfg_error_count++;
      return 1;
    }
  addr.s_addr = ntohl (addr.s_addr);

  *ptr = addr;
  return 0;
}
#endif

static int
to_cidr (void *tgt, char const *string, char **errmsg)
{
  struct mu_cidr *ptr = tgt;
  return mu_cidr_from_string (ptr, string);
}

static int
to_incr (void *tgt, char const *string, char **errmsg)
{
  ++*(int*)tgt;
  return 0;
}

static str_to_c_t str_to_c[] = {
  [mu_c_string] = to_string, 
  [mu_c_short]  = to_short,  
  [mu_c_ushort] = to_ushort, 
  [mu_c_int]    = to_int,    
  [mu_c_uint]   = to_uint,   
  [mu_c_long]   = to_long,   
  [mu_c_ulong]  = to_ulong,  
  [mu_c_size]   = to_size_t,   
  /* FIXME  [mu_c_off]    = { to_off,    generic_dealloc }, */
  [mu_c_time]   = to_time_t, 
  [mu_c_bool]   = to_bool,
  /* FIXME [mu_c_ipv4]   = to_ipv4, */
  [mu_c_cidr]   = to_cidr,
  /* FIXME  [mu_c_host]   = { to_host,   generic_dealloc } */
  [mu_c_incr]   = to_incr
};

char const *mu_c_type_str[] = {
  [mu_c_string]  = "mu_c_string",
  [mu_c_short]   = "mu_c_short",  
  [mu_c_ushort]  = "mu_c_ushort", 
  [mu_c_int]     = "mu_c_int",    
  [mu_c_uint]    = "mu_c_uint",   
  [mu_c_long]    = "mu_c_long",   
  [mu_c_ulong]   = "mu_c_ulong",  
  [mu_c_size]    = "mu_c_size",   
  [mu_c_off]     = "mu_c_off",    
  [mu_c_time]    = "mu_c_time",   
  [mu_c_bool]    = "mu_c_bool",   
  [mu_c_ipv4]    = "mu_c_ipv4",   
  [mu_c_cidr]    = "mu_c_cidr",   
  [mu_c_host]    = "mu_c_host",   
  [mu_c_incr]    = "mu_c_incr",   
  [mu_c_void]    = "mu_c_void"
};

int
mu_str_to_c (char const *string, enum mu_c_type type, void *tgt, char **errmsg)
{
  if (errmsg)
    *errmsg = NULL;
  if ((size_t)type >= sizeof (str_to_c) / sizeof (str_to_c[0]))
    return EINVAL;
  if (!str_to_c[type])
    return ENOSYS;
  return str_to_c[type] (tgt, string, errmsg);
}
