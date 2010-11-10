/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2009,
   2010 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

/* MH context functions. */
  
#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>

mh_context_t *
mh_context_create (const char *name, int copy)
{
  mh_context_t *ctx;
  ctx = malloc (sizeof (*ctx));
  if (!ctx)
    mh_err_memory (1);
  if (copy)
    ctx->name = name;
  else
    {
      ctx->name = strdup (name);
      if (!ctx->name)
        mh_err_memory (1);
    }
  ctx->header = NULL;
  return ctx;
}

void
mh_context_destroy (mh_context_t **pctx)
{
  mh_context_t *ctx = *pctx;
  
  free ((char*) ctx->name);
  if (ctx->header)
    mu_header_destroy (&ctx->header);
  free (ctx);
  *pctx = NULL;
}

void
mh_context_merge (mh_context_t *dst, mh_context_t *src)
{
  if (!dst->header)
    {
      dst->header = src->header;
      src->header = NULL;
    }
  else
    {
      size_t i, count;
      
      mu_header_get_field_count (src->header, &count);
      for (i = 1; i <= count; i++)
	{
	  const char *name = NULL;
	  const char *value = NULL;
	  
	  mu_header_sget_field_name (src->header, i, &name);
	  mu_header_sget_field_value (src->header, i, &value);
	  mu_header_set_value (dst->header, name, value, 1);
	}
    }
}

int 
mh_context_read (mh_context_t *ctx)
{
  int rc;
  char *blurb, *p;
  mu_stream_t stream;
  mu_off_t stream_size;
  char *buf = NULL;
  size_t size = 0, n;
  
  if (!ctx)
    return MU_ERR_OUT_NULL;
  
  rc = mu_file_stream_create (&stream, ctx->name, MU_STREAM_READ);
  if (rc)
    return rc;

  rc = mu_stream_size (stream, &stream_size);
  if (rc)
    {
      mu_stream_destroy (&stream);
      return rc;
    }

  blurb = malloc (stream_size + 1);
  if (!blurb)
    {
      mu_stream_destroy (&stream);
      return ENOMEM;
    }
  
  p = blurb;
  while (mu_stream_getline (stream, &buf, &size, &n) == 0 && n > 0)
    {
      char *q = mu_str_skip_class (buf, MU_CTYPE_SPACE);
      if (!*q || *q == '#')
	continue;
      for (q = buf; *q;)
	*p++ = *q++;
    }
  mu_stream_destroy (&stream);
  rc = mu_header_create (&ctx->header, blurb, p - blurb);
  free (blurb);

  return rc;
}

int 
mh_context_write (mh_context_t *ctx)
{
  int rc;
  mu_stream_t instream, outstream;
  mu_off_t size;
  
  if (!ctx)
    return MU_ERR_OUT_NULL;

  rc = mu_file_stream_create (&outstream, ctx->name,
			      MU_STREAM_WRITE|MU_STREAM_CREAT);
  if (rc)
    {
      mu_error (_("cannot open context file %s for writing: %s"),
		ctx->name, mu_strerror (rc));
      return MU_ERR_FAILURE;
    }
  
  mu_header_get_streamref (ctx->header, &instream);
  rc = mu_stream_copy (outstream, instream, 0, &size);
  if (rc)
    {
      mu_error (_("error writing to context file %s: %s"),
		ctx->name, mu_strerror (rc));
      return MU_ERR_FAILURE;
    }
  else
    rc = mu_stream_truncate (outstream, size);
  mu_stream_destroy (&instream);
  mu_stream_destroy (&outstream);
  return 0;
}

const char *
mh_context_get_value (mh_context_t *ctx, const char *name, const char *defval)
{
  const char *p;

  if (!ctx || mu_header_sget_value (ctx->header, name, &p))
    p = defval; 
  return p;
}

int
mh_context_set_value (mh_context_t *ctx, const char *name, const char *value)
{
  if (!ctx)
    return EINVAL;
  if (!ctx->header)
    {
      int rc;
      if ((rc = mu_header_create (&ctx->header, NULL, 0)) != 0)
	{
	  mu_error (_("cannot create context %s: %s"),
		    ctx->name,
		    mu_strerror (rc));
	  return 1;
	}
    }
  return mu_header_set_value (ctx->header, name, value, 1);
}

int
mh_context_iterate (mh_context_t *ctx, mh_context_iterator fp, void *data)
{
  size_t i, nfields;
  int rc = 0;
  
  if (!ctx)
    return EINVAL;
  if (!ctx->header)
    return 0;
  rc = mu_header_get_field_count (ctx->header, &nfields);
  if (rc)
    {
      mu_error (_("cannot obtain field count for context %s"), ctx->name);
      return rc;
    }
  
  for (i = 1; i <= nfields && rc == 0; i++)
    {
      const char *name, *value;
      
      rc = mu_header_sget_field_name (ctx->header, i, &name);
      if (rc)
	{
	  mu_error (_("cannot obtain field name for context %s:%d: %s"),
		    ctx->name,i,mu_strerror (rc));
	  break;
	}
      
      rc = mu_header_sget_field_value (ctx->header, i, &value);
      if (rc)
	{
	  mu_error (_("cannot obtain field value for context %s:%d: %s"),
		    ctx->name,i,mu_strerror (rc));
	  break;
	}
      
      rc = fp (name, value, data);
    }

  return rc;
}
