/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>

#include <mailutils/wordsplit.h>
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/argcv.h>
#include <mailutils/secret.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/url.h>

struct mu_url_ctx
{
  int flags;
  const char *input;
  const char *cur;
  mu_url_t url;

  size_t passoff;
  
  char *tokbuf;
  size_t toksize;
  size_t toklen;
};

static int
getkn (struct mu_url_ctx *ctx, char *delim)
{
  size_t n;

  if (*ctx->cur == 0)
    return MU_ERR_PARSE;
  n = strcspn (ctx->cur, delim);
  if (n + 1 > ctx->toksize)
    {
      char *p = realloc (ctx->tokbuf, n + 1);
      if (!p)
	return ENOENT;
      ctx->toksize = n + 1;
      ctx->tokbuf = p;
    }
  memcpy (ctx->tokbuf, ctx->cur, n);
  ctx->tokbuf[n] = 0;
  ctx->toklen = n;
  ctx->cur += n;
  return 0;
}

#define INIT_ARRAY_SIZE 16

static int
expand_array (size_t *pwc, char ***pwv, int incr)
{
  size_t wc = *pwc;
  char **wv = *pwv;

  if (!wv)
    {
      wv = calloc (INIT_ARRAY_SIZE, sizeof (wv[0]));
      wc = INIT_ARRAY_SIZE;
    }
  else
    {
      if (incr)
	wc += incr;
      else
	{
	  size_t newsize = wc * 2;
	  if (newsize < wc)
	    return ENOMEM;
	  wc = newsize;
	}
      wv = realloc (wv, sizeof (wv[0]) * wc);
    }
  if (!wv)
    return ENOMEM;
  *pwv = wv;
  *pwc = wc;
  return 0;
}
  
static int
parse_param (struct mu_url_ctx *ctx, char *delim, int *pargc, char ***pargv)
{
  int rc;
  size_t wc = 0, wn = 0;
  char **wv = NULL;

  while ((rc = getkn (ctx, delim)) == 0)
    {
      if (wn == wc)
	{
	  rc = expand_array (&wc, &wv, 0);
	  if (rc)
	    break;
	}
      wv[wn] = strdup (ctx->tokbuf);
      if (!wv[wn])
	{
	  rc = ENOMEM;
	  break;
	}
      wn++;
      if (*ctx->cur != delim[0])
	break;
      ctx->cur++;
    }

  if (rc == 0)
    {
      if (wn == wc)
	{
	  rc = expand_array (&wc, &wv, 1);
	  if (rc)
	    {
	      mu_argcv_free (wc, wv);
	      return ENOMEM;
	    }
	  wv[wn] = NULL;
	}

      *pargv = realloc (wv, sizeof (wv[0]) * (wn + 1));
      *pargc = wn;
    }
  else
    mu_argcv_free (wc, wv);

  return rc;
}

static int
_mu_url_ctx_parse_query (struct mu_url_ctx *ctx)
{
  int rc;
  
  ctx->cur++;
  rc = parse_param (ctx, "&", &ctx->url->qargc, &ctx->url->qargv);
  if (rc == 0 && ctx->url->qargc)
    ctx->url->flags |= MU_URL_QUERY;
  return rc;
}

static int
_mu_url_ctx_parse_param (struct mu_url_ctx *ctx)
{
  int rc;
  
  ctx->cur++;
  rc = parse_param (ctx, ";?", &ctx->url->fvcount, &ctx->url->fvpairs);
  if (rc)
    return rc;
  if (ctx->url->fvcount)
    ctx->url->flags |= MU_URL_PARAM;
  if (*ctx->cur == '?')
    return _mu_url_ctx_parse_query (ctx);
  return 0;
}

static int
str_assign (char **ptr, const char *str)
{
  *ptr = strdup (str);
  if (!*ptr)
    return ENOMEM;
  return 0;
}

static int
_mu_url_ctx_parse_path (struct mu_url_ctx *ctx)
{
  int rc;
  mu_url_t url = ctx->url;
  
  rc = getkn (ctx, ";?");
  if (rc)
    return rc;
  rc = str_assign (&url->path, ctx->tokbuf);
  if (rc == 0)
    url->flags |= MU_URL_PATH;
  if (*ctx->cur == ';')
    return _mu_url_ctx_parse_param (ctx);
  if (*ctx->cur == '?')
    return _mu_url_ctx_parse_query (ctx);
  return 0;
}

static int
_mu_url_ctx_parse_host (struct mu_url_ctx *ctx, int has_host)
{
  int rc;
  mu_url_t url = ctx->url;
  
  if (ctx->flags & MU_URL_PARSE_LOCAL)
    return _mu_url_ctx_parse_path (ctx);
  
  rc = getkn (ctx, "[:/;?");
  if (rc)
    return rc;

  if (*ctx->cur == '[')
    {
      /* Possibly IPv6 address */
      rc = getkn (ctx, "]/;?");
      if (rc)
	return rc;
      if (*ctx->cur == ']')
	{
	  ctx->cur++;
	  rc = str_assign (&url->host, ctx->tokbuf + 1);
	  if (rc)
	    return rc;
	  url->flags |= MU_URL_HOST | MU_URL_IPV6;
	  has_host = 1;
	}
    }

  if (!(url->flags & MU_URL_HOST) && ctx->toklen)
    {
      rc = str_assign (&url->host, ctx->tokbuf);
      if (rc)
	return rc;
      url->flags |= MU_URL_HOST;
      has_host = 1;
    }
  
  if (*ctx->cur == ':')
    {
      has_host = 1;
      ctx->cur++;
      rc = getkn (ctx, ":/;?");
      if (rc)
	return rc;
      rc = str_assign (&url->portstr, ctx->tokbuf);
      if (rc)
	return rc;
      url->flags |= MU_URL_PORT;
    }
    
  if (*ctx->cur == '/')
    {
      if (has_host)
	ctx->cur++;
      return _mu_url_ctx_parse_path (ctx);
    }
  
  if (*ctx->cur == ';')
    return _mu_url_ctx_parse_param (ctx);

  if (*ctx->cur == '?')
    return _mu_url_ctx_parse_query (ctx);
  return 0;
}

static int
_mu_url_ctx_parse_cred (struct mu_url_ctx *ctx)
{
  int rc, has_cred;
  mu_url_t url = ctx->url;
  const char *save = ctx->cur;

  if (*ctx->cur == 0)
    return 0;
  rc = getkn (ctx, "@");
  if (rc)
    return rc;
  has_cred = *ctx->cur == '@';
  /* restore the pointer */
  ctx->cur = save;  
  if (has_cred)
    {
      /* Try to split the user into a:
	 <user>:<password>
	 or
	 <user>:<password>;AUTH=<auth>
      */
      rc = getkn (ctx, ":;@");
      if (rc)
	return rc;

      if (ctx->toklen)
	{
	  rc = str_assign (&url->user, ctx->tokbuf);
	  if (rc)
	    return rc;
	  url->flags |= MU_URL_USER;
	}
      
      if (*ctx->cur == ':')
	{
	  ctx->cur++;
	  ctx->passoff = ctx->cur - ctx->input;
	  
	  rc = getkn (ctx, ";@");
	  if (rc)
	    return rc;

	  if (ctx->toklen)
	    {
	      if (mu_secret_create (&url->secret, ctx->tokbuf, ctx->toklen))
		return ENOMEM;
	      else
		/* Clear password */
		memset (ctx->tokbuf, 0, ctx->toklen);
	      url->flags |= MU_URL_SECRET;
	    }
	}
      if (*ctx->cur == ';')
	{
	  ctx->cur++;

	  rc = getkn (ctx, "@");
	  if (rc)
	    return rc;
	  
	  /* Make sure it's the auth token. */
	  if (mu_c_strncasecmp (ctx->tokbuf, "auth=", 5) == 0)
	    {
	      rc = str_assign (&url->auth, ctx->tokbuf + 5);
	      if (rc)
		return rc;
	      url->flags |= MU_URL_AUTH;
	    }
	}

      /* Skip @ sign */
      ctx->cur++;
    }
  return _mu_url_ctx_parse_host (ctx, has_cred);
}

int
_mu_url_ctx_parse (struct mu_url_ctx *ctx)
{
  int rc;
  mu_url_t url = ctx->url;
  const char *save = ctx->cur;
  
  /* Parse the scheme part */
  if (*ctx->cur == ':')
    return _mu_url_ctx_parse_cred (ctx);
  
  rc = getkn (ctx, ":/");
  if (rc)
    return rc;
  if (*ctx->cur == ':'
      && ((ctx->flags & MU_URL_PARSE_DSLASH_OPTIONAL)
	  || (ctx->cur[1] == '/' && ctx->cur[2] == '/')))
    {
      rc = str_assign (&url->scheme, ctx->tokbuf);
      if (rc)
	return rc;
      url->flags |= MU_URL_SCHEME;
      ctx->cur++;
    }
  else
    {
      ctx->cur = save;
      return _mu_url_ctx_parse_cred (ctx);
    }
  
  if (*ctx->cur == 0)
    return 0;

  if (ctx->cur[0] == '/' && ctx->cur[1] == '/')
    {
      ctx->cur += 2;
      return _mu_url_ctx_parse_cred (ctx);
    }

  return _mu_url_ctx_parse_path (ctx);
}

static int
_mu_url_create_internal (struct mu_url_ctx *ctx, mu_url_t hint)
{
  int rc = 0;
  mu_url_t url = ctx->url;
  const char *scheme = NULL;
  
  if ((ctx->flags & MU_URL_PARSE_PIPE) && ctx->input[0] == '|')
    {
      struct mu_wordsplit ws;
      size_t i;
      
      scheme = "prog";
      ctx->flags &= ~MU_URL_PARSE_HEXCODE;
      if (mu_wordsplit (ctx->input + 1, &ws, MU_WRDSF_DEFFLAGS))
	return errno;
      url->path = ws.ws_wordv[0];
      url->flags |= MU_URL_PATH;
      
      url->qargc = ws.ws_wordc - 1;
      url->qargv = calloc (url->qargc + 1, sizeof (url->qargv[0]));
      if (!url->qargv)
	{
	  mu_wordsplit_free (&ws);
	  return ENOMEM;
	}
      for (i = 0; i < url->qargc; i++)
	url->qargv[i] = ws.ws_wordv[i+1];
      ws.ws_wordc = 0;
      mu_wordsplit_free (&ws);
      url->flags |= MU_URL_QUERY;
    }
  else if ((ctx->flags & MU_URL_PARSE_SLASH) &&
	   (ctx->input[0] == '/' ||
	    (ctx->input[0] == '.' && ctx->input[1] == '/')))
    {
      scheme = "file";
      ctx->flags &= ~MU_URL_PARSE_HEXCODE;
      rc = str_assign (&url->path, ctx->input);
      if (rc == 0)
	url->flags |= MU_URL_PATH;
    }
  else
    rc = _mu_url_ctx_parse (ctx);
  
  if (rc)
    return rc;

  if (hint)
    {
      /* Fill in missing values */
      rc = mu_url_copy_hints (url, hint);
      if (rc)
	return rc;
    }

  if (!(url->flags & MU_URL_SCHEME))
    {
      if (scheme)
	{
	  rc = str_assign (&url->scheme, scheme);
	  if (rc)
	    return rc;
	  url->flags |= MU_URL_SCHEME;
	}
      else
	return MU_ERR_URL_MISS_PARTS;
    }
  
  /* RFC 1738, section 2.1, lower the scheme case */
  mu_strlower (url->scheme);

  if ((url->flags & MU_URL_PORT) && url->port == 0)
    {
      /* Convert port string to number */
      unsigned long n;
      char *p;
      
      n = strtoul (url->portstr, &p, 10);
      if (*p)
	{
	  if (ctx->flags & MU_URL_PARSE_PORTSRV)
	    {
	      /* FIXME: Another proto? */
	      struct servent *sp = getservbyname (url->portstr, "tcp");
	      if (!sp)
		return MU_ERR_TCP_NO_PORT; 
	      url->port = ntohs (sp->s_port);
	    }
	  else
	    return MU_ERR_TCP_NO_PORT;
	}
      else if (n > USHRT_MAX)
	return ERANGE;
      else
	url->port = n;
    }

  if (ctx->flags & MU_URL_PARSE_HEXCODE)
    {
      /* Decode the %XX notations */
      rc = mu_url_decode (url);
      if (rc)
	return rc;
    }
  
  if ((url->flags & MU_URL_SECRET) &&
      (ctx->flags & MU_URL_PARSE_HIDEPASS))
    {
      /* Obfuscate the password */
#define PASS_REPL "***"
#define PASS_REPL_LEN (sizeof (PASS_REPL) - 1)
      size_t plen = mu_secret_length (url->secret);
      size_t nlen = strlen (url->name);
      size_t len = nlen - plen + PASS_REPL_LEN + 1;
      char *newname;
      
      memset (url->name + ctx->passoff, 0, plen);
      if (len > nlen + 1)
	{
	  newname = realloc (url->name, len);
	  if (!newname)
	    return rc;
	  url->name = newname;
	}
      else
	newname = url->name;
      memmove (newname + ctx->passoff + PASS_REPL_LEN,
	       newname + ctx->passoff + plen,
	       nlen - (ctx->passoff + plen) + 1);
      memcpy (newname + ctx->passoff, PASS_REPL, PASS_REPL_LEN);
    }

  return 0;
}

int
mu_url_create_hint (mu_url_t *purl, const char *str, int flags,
		    mu_url_t hint)
{
  int rc;
  struct mu_url_ctx ctx;
  mu_url_t url;

  if (!purl)
    return EINVAL;
  url = calloc (1, sizeof (*url));
  if (url == NULL)
    return ENOMEM;
  url->name = strdup (str);
  if (!url->name)
    {
      free (url);
      return ENOMEM;
    }
  memset (&ctx, 0, sizeof (ctx));
  ctx.flags = flags;
  ctx.input = str;
  ctx.cur = ctx.input;
  ctx.url = url;
  rc = _mu_url_create_internal (&ctx, hint);
  free (ctx.tokbuf);
  if (rc)
    mu_url_destroy (&url);
  else
    *purl = url;
  return rc;
}
      
int
mu_url_create (mu_url_t *purl, const char *str)
{
  return mu_url_create_hint (purl, str,
			     MU_URL_PARSE_HEXCODE |
			     MU_URL_PARSE_HIDEPASS |
			     MU_URL_PARSE_PORTSRV |
			     MU_URL_PARSE_PIPE |
			     MU_URL_PARSE_SLASH |
			     MU_URL_PARSE_DSLASH_OPTIONAL, NULL);
}
