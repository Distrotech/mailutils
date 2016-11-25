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

/* The enchoded-character extension for Sieve (RFC 5228, 2.4.2.4) */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <sieve-priv.h>
#include <mailutils/cctype.h>

typedef int (*convfun) (char const *str, size_t len, size_t *ncons, mu_opool_t pool);

static int hexconv (char const *str, size_t len, size_t *ncons, mu_opool_t pool);
static int uniconv (char const *str, size_t len, size_t *ncons, mu_opool_t pool);

struct convertor
{
  char const *pfx;
  size_t len;
  convfun fun;
};

static struct convertor conv[] = {
  { "hex",     3, hexconv },
  { "unicode", 7, uniconv },
  { NULL }
};

static convfun
findconv (char const **pstr, size_t *plen)
{
  struct convertor *cp;
  char const *str = *pstr;
  size_t len = *plen;
  
  for (cp = conv; cp->pfx; cp++)
    {
      if (len > cp->len && strncasecmp (str, cp->pfx, cp->len) == 0 &&
	  str[cp->len] == ':')
	{
	  *pstr += cp->len + 1;
	  *plen -= cp->len + 1;
	  return cp->fun;
	}
    }
  return NULL;
}

int
mu_i_sv_expand_encoded_char (char const *input, size_t len,
			     char **exp, void *data)
{
  int rc;
  convfun fn;
  mu_opool_t pool;
  
  fn = findconv (&input, &len);
  if (!fn)
    return MU_ERR_NOENT;

  rc = mu_opool_create (&pool, MU_OPOOL_DEFAULT);
  if (rc)
    return rc;

  while (rc == 0 && len > 0)
    {
      if (mu_isblank (*input))
	{
	  ++input;
	  --len;
	}
      else if (mu_isxdigit (*input))
	{
	  size_t n;
	  rc = fn (input, len, &n, pool);
	  if (rc)
	    break;
	  input += n;
	  len -= n;
	}
      else
	{
	  rc = EILSEQ;
	  break;
	}
    }

  if (rc == 0)
    {
      size_t len;
      char *p = mu_opool_finish (pool, &len);
      char *res;

      res = malloc (len + 1);
      if (!res)
	rc = errno;
      else
	{
	  memcpy (res, p, len);
	  res[len] = 0;
	  *exp = res;
	}
    }
  mu_opool_destroy (&pool);
  return rc;
}

static int
hexconv (char const *str, size_t len, size_t *ncons, mu_opool_t pool)
{
  char c;
  
  if (len < 2)
    return EILSEQ;
  else 
    {
      c = mu_hex2ul (*str);
      ++str;
      if (!mu_isxdigit (*str))
	return EILSEQ;
      c = (c << 4) + mu_hex2ul (*str);
      mu_opool_append_char (pool, c);
    }
  *ncons = 2;
  return 0;
}

static int
utf8_wctomb (unsigned int wc, mu_opool_t pool)
{
  int count;
  char r[6];

  /* FIXME: This implementation allows for full UTF-8 range.  RFC 5228
     states on page 10, that "It is an error for a script to use a hexadecimal
     value that isn't in either the range 0 to D7FF or the range E000 to
     10FFFF".  I'm not sure that this limitation should be honored */
     
  if (wc < 0x80)
    count = 1;
  else if (wc < 0x800)
    count = 2;
  else if (wc < 0x10000)
    count = 3;
  else if (wc < 0x200000)
    count = 4;
  else if (wc < 0x4000000)
    count = 5;
  else if (wc <= 0x7fffffff)
    count = 6;
  else
    return EILSEQ;

  switch (count)
    {
      /* Note: code falls through cases! */
    case 6:
      r[5] = 0x80 | (wc & 0x3f);
      wc = wc >> 6;
      wc |= 0x4000000;
    case 5:
      r[4] = 0x80 | (wc & 0x3f);
      wc = wc >> 6;
      wc |= 0x200000;
    case 4:
      r[3] = 0x80 | (wc & 0x3f);
      wc = wc >> 6;
      wc |= 0x10000;
    case 3:
      r[2] = 0x80 | (wc & 0x3f);
      wc = wc >> 6;
      wc |= 0x800;
    case 2:
      r[1] = 0x80 | (wc & 0x3f);
      wc = wc >> 6;
      wc |= 0xc0;
    case 1:
      r[0] = wc;
    }
  mu_opool_append (pool, r, count);
  return 0;
}

static int
uniconv (char const *str, size_t len, size_t *ncons, mu_opool_t pool)
{
  unsigned int wc = 0;
  size_t i;
  
  for (i = 0; i < len; i++)
    {
      if (i >= 12)
	return EILSEQ;
      if (!mu_isxdigit (str[i]))
	break;
      wc = (wc << 4) + mu_hex2ul (str[i]);
    }
  *ncons = i;
  return utf8_wctomb (wc, pool);
}

