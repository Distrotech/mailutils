/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* MH context functions. */
  
#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>

mh_context_t *
mh_context_create (char *name, int copy)
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

int 
mh_context_read (mh_context_t *ctx)
{
  int status;
  char *blurb;
  struct stat st;
  FILE *fp;

  if (stat (ctx->name, &st))
    return errno;

  blurb = malloc (st.st_size);
  if (!blurb)
    return ENOMEM;
  
  fp = fopen (ctx->name, "r");
  if (!fp)
    {
      free (blurb);
      return errno;
    }

  fread (blurb, st.st_size, 1, fp);
  fclose (fp);
  
  if ((status = header_create (&ctx->header, blurb, st.st_size, NULL)) != 0) 
    free (blurb);

  return status;
}

int 
mh_context_write (mh_context_t *ctx)
{
  stream_t stream;
  char buffer[512];
  size_t off = 0, n;
  FILE *fp;
  
  fp = fopen (ctx->name, "w");
  if (!fp)
    {
      mh_error (_("can't write context file %s: %s"),
		ctx->name, strerror (errno));
      return 1;
    }
  
  header_get_stream (ctx->header, &stream);

  while (stream_read (stream, buffer, sizeof buffer - 1, off, &n) == 0
	 && n != 0)
    {
      buffer[n] = '\0';
      fprintf (fp, "%s", buffer);
      off += n;
    }

  fclose (fp);
  return 0;
}

/* FIXME: mh_context_get_value returns a pointer to the allocated memory.
   Instead, it should return a const pointer to the static storage within
   the header_t structure and be declared as
   `const char *mh_context_get_value()'. Current implementation of
   header_.* functions does not allow that.

   This has two drawbacks:
     1) The function is declared as returning char * instead of
        intended const char *.
     2) Ugly typecast when returning defval. */
  
char *
mh_context_get_value (mh_context_t *ctx, const char *name, const char *defval)
{
  char *p;

  if (header_aget_value (ctx->header, name, &p))
    p = (char *) defval; 
  return p;
}

int
mh_context_set_value (mh_context_t *ctx, const char *name, const char *value)
{
  if (!ctx->header)
    {
      int rc;
      if ((rc = header_create (&ctx->header, NULL, 0, NULL)) != 0)
	{
	  mh_error (_("Can't create context %s: %s"),
		    ctx->name,
		    mu_errstring (rc));
	  return 1;
	}
    }
  return header_set_value (ctx->header, name, value, 1);
}
