/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/stream.h>
#include <mailutils/address.h>
#include <mailutils/mutil.h>
#include <mailutils/errno.h>

#include <header0.h>

#define HEADER_MODIFIED 1

static int header_parse    (mu_header_t, const char *, int);
static int header_read     (mu_stream_t, char *, size_t, mu_off_t, size_t *);
static int header_readline (mu_stream_t, char *, size_t, mu_off_t, size_t *);
static int header_write    (mu_stream_t, const char *, size_t, mu_off_t, size_t *);
static int fill_blurb      (mu_header_t);
static void header_free_cache (mu_header_t);

int
mu_header_create (mu_header_t *ph, const char *blurb, size_t len, void *owner)
{
  mu_header_t header;
  int status = 0;

  header = calloc (1, sizeof (*header));
  if (header == NULL)
    return ENOMEM;

  header->owner = owner;

  status = header_parse (header, blurb, len);

  *ph = header;
  return status;
}

static void
header_free_cache (mu_header_t header)
{
  /* Clean up our fast header cache.  */
  if (header->fhdr)
    {
      size_t i;
      for (i = 0; i < header->fhdr_count; i++)
	{
	  if (header->fhdr[i].fn)
	    free (header->fhdr[i].fn);
	  if (header->fhdr[i].fv)
	    free (header->fhdr[i].fv);
	}
      free (header->fhdr);
      header->fhdr = NULL;
      header->fhdr_count = 0;
    }
}

void
mu_header_destroy (mu_header_t *ph, void *owner)
{
  if (ph && *ph)
    {
      mu_header_t header = *ph;

      /* Can we destroy ?.  */
      if (header->owner == owner)
	{
	  mu_stream_destroy (&(header->stream), header);

	  if (header->hdr)
	    free (header->hdr);

	  if (header->blurb)
	    free (header->blurb);

	  header_free_cache (header);

	  if (header->mstream)
	    mu_stream_destroy (&(header->mstream), NULL);

	  free (header);
	}
      *ph = NULL;
    }
}

void *
mu_header_get_owner (mu_header_t header)
{
  return (header) ? header->owner : NULL;
}

int
mu_header_is_modified (mu_header_t header)
{
  return (header) ? (header->flags & HEADER_MODIFIED) : 0;
}

int
mu_header_clear_modified (mu_header_t header)
{
  if (header)
    header->flags &= ~HEADER_MODIFIED;
  return 0;
}

/* Parsing is done in a rather simple fashion, meaning we just consider an
   entry to be a field-name an a field-value.  So they maybe duplicate of
   field-name like "Received" they are just put in the array, see _get_value()
   on how to handle the case. in the case of error .i.e a bad header construct
   we do a full stop and return what we have so far.  */
static int
header_parse (mu_header_t header, const char *blurb, int len)
{
  char *header_end;
  char *header_start;
  char *header_start2;
  struct _hdr *hdr;

  /* Nothing to parse.  */
  if (blurb == NULL)
    return 0;

  header->blurb_len = len;
  /* Why "+ 1", if for a terminating NULL, where is written? */
  header->blurb = calloc (1, header->blurb_len + 1);
  if (header->blurb == NULL)
    return ENOMEM;
  memcpy (header->blurb, blurb, header->blurb_len);

  if (header->hdr)
    free (header->hdr);
  header->hdr = NULL;
  header->hdr_count = 0;

  /* Get a header, a header is:
     field-name LWSP ':'
       LWSP field-value '\r' '\n'
       *[ (' ' | '\t') field-value '\r' '\n' ]
  */
  /* First loop goes through the blurb */
  for (header_start = header->blurb;  ; header_start = ++header_end)
    {
      char *fn, *fn_end, *fv, *fv_end;

      if (header_start[0] == ' '
	  || header_start[0] == '\t'
	  || header_start[0] == '\n')
	break;

      /* Second loop extract one header field. */
      for (header_start2 = header_start;  ;header_start2 = ++header_end)
	{
	  header_end = memchr (header_start2, '\n', len);
	  if (header_end == NULL)
	    break;
	  else
	    {
	      len -= (header_end - header_start2 + 1);
	      if (len < 0)
		{
		  header_end = NULL;
		  break;
		}
	      if (header_end[1] != ' '
		  && header_end[1] != '\t')
		break; /* New header break the inner for. */
	    }
	  /* *header_end = ' ';  smash LF ? NO */
	}

      if (header_end == NULL)
	break; /* Bail out.  */

      /* Now save the header in the data structure.  */

      /* Treats unix "From " specially.  */
      if ((header_end - header_start >= 5)
      	  && strncmp (header_start, "From ", 5) == 0)
	{
	  fn = header_start;
	  fn_end = header_start + 5;
	  fv = header_start + 5;
	  fv_end = header_end;
	}
      else /* Break the header in key: value */
	{
	  char *colon = memchr (header_start, ':', header_end - header_start);

#define ISLWSP(c) (((c) == ' ' || (c) == '\t'))
	  /* Houston we have a problem.  */
	  if (colon == NULL)
	    break; /* Disregard the rest and bailout.  */

	  fn = header_start;
	  fn_end = colon;
	  /* Shrink any LWSP after the field name -- CRITICAL for 
	   later name comparisons to work correctly! */
	  while(ISLWSP(fn_end[-1]))
	    fn_end--;

	  fv = colon + 1;
	  fv_end = header_end;

	  /* Skip any LWSP before the field value -- unnecessary, but
	   might make some field values look a little tidier. */
	  while(ISLWSP(fv[0]))
	    fv++;
	}
#undef ISLWSP
      /* Allocate a new slot for the field:value.  */
      hdr = realloc (header->hdr, (header->hdr_count + 1) * sizeof (*hdr));
      if (hdr == NULL)
	{
	  free (header->blurb);
	  free (header->hdr);
	  header->blurb = NULL;
	  header->hdr = NULL;
	  return ENOMEM;
	}
      hdr[header->hdr_count].fn = fn;
      hdr[header->hdr_count].fn_end = fn_end;
      hdr[header->hdr_count].fv = fv;
      hdr[header->hdr_count].fv_end = fv_end;
      header->hdr = hdr;
      header->hdr_count++;
    } /* for (header_start ...) */

 return 0;
}

/* FIXME: grossly inneficient, to many copies and reallocating.
   This all header business need a good rewrite.  */
int
mu_header_set_value (mu_header_t header, const char *fn, const char *fv,
		     int replace)
{
  char *blurb;
  size_t len;

  if (header == NULL || fn == NULL)
    return EINVAL;

  /* An fv of NULL means delete the field, but only do it if replace
     was also set to true! */
  if (fv == NULL && !replace)
    return EINVAL;

  /* Overload.  */
  if (header->_set_value)
    return header->_set_value (header, fn, fv, replace);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  /* Easy approach: if replace, overwrite the field-{name,value} and readjust
     the pointers by calling header_parse () this is wastefull, we're just
     fragmenting the memory it can be done better.  But that may imply a
     rewite of the headers ... for another day.  */

  /* If replace, remove all fields in the header blurb that have the
     same name as the field we are writing.

     Algorithm:

     for i = 0, ... i < max_hdrs
     - if ith field has name 'fn' memmove() all following fields up over
     this field
     - reparse the headers
     - restart the for loop at the ith field
   */
  if (replace)
    {
      size_t name_len;
      size_t i;
      size_t fn_len;		/* Field Name len.  */
      size_t fv_len;		/* Field Value len.  */
      len = header->blurb_len;
      /* Find FN in the header fields... */
      for (name_len = strlen (fn), i = 0; i < header->hdr_count; i++)
	{
	  fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
	  fv_len = header->hdr[i].fv_end - header->hdr[i].fv;
	  if (fn_len == name_len &&
	      strncasecmp (header->hdr[i].fn, fn, fn_len) == 0)
	    {
	      blurb = header->blurb;
	      /* ... and if its NOT the last field, move the next field
	         through to last field into its place, */
	      if ((i + 1) < header->hdr_count)
		{
		  memmove (header->hdr[i].fn, header->hdr[i + 1].fn,
			   header->hdr[header->hdr_count - 1].fv_end
			   - header->hdr[i + 1].fn + 3);
		}
	      /* or if it is the last, just truncate the fields. */
	      else
		{
		  header->hdr[i].fn[0] = '\n';
		  header->hdr[i].fn[1] = '\0';
		}
	      /* Readjust the pointers. */
	      len -= header->hdr[i].fv_end  - header->hdr[i].fn + 1;
	      i--;
	      blurb = header->blurb;
	      header_parse (header, blurb, len);
	      free (blurb);
	      header->flags |= HEADER_MODIFIED;
	    }
	}
    }

  /* If FV is NULL, then we are done. */
  if (!fv)
    return 0;

  /* Replacing was taken care of above, now write the new header.
     header.  Really not cute.
     COLON SPACE NL =  3 ;  */
  len = strlen (fn) + strlen (fv) + 3;
  /* Add one for the NULL and leak a bit by adding one more
     it will be the separtor \n from the body if the first
     blurb did not have it.  */
  blurb = calloc (header->blurb_len + len + 2, 1);
  if (blurb == NULL)
    return ENOMEM;

  sprintf (blurb, "%s: %s", fn, fv);

  /* Strip off trailing newlines and LWSP. */
  while (blurb[strlen (blurb) - 1] == '\n' ||
	 blurb[strlen (blurb) - 1] == ' ' ||
	 blurb[strlen (blurb) - 1] == '\t')
    {
      blurb[strlen (blurb) - 1] = '\0';
    }
  len = strlen (blurb);
  blurb[len] = '\n';
  len++;

  /* Prepend the rest of the headers.  */
  if (header->blurb)
    {
      memcpy (blurb + len, header->blurb, header->blurb_len);
      free (header->blurb);
      header->blurb = NULL;
    }
  else
    blurb[len] = '\n';

  /* before parsing the new blurb make sure it is properly terminated
     by \n\n. The trailing NL separator.  */
  if (blurb[header->blurb_len + len - 1] != '\n'
      || blurb[header->blurb_len + len - 2] != '\n')
    {
      blurb[header->blurb_len + len] = '\n';
      len++;
    }
  header_parse (header, blurb, len + header->blurb_len);
  free (blurb);
  header->flags |= HEADER_MODIFIED;
  return 0;
}

/* We try to cache the headers here to reduce networking access
   especially for IMAP.  When the buffer is NULL it means that
   the field does not exist on the server and we should not
   attempt to contact the server again for this field.  */
static int
header_set_fvalue (mu_header_t header, const char *name, char *buffer)
{
  struct _hdr *thdr;
  thdr = realloc (header->fhdr, (header->fhdr_count + 1) * sizeof(*thdr));
  if (thdr)
    {
      size_t len = strlen (name);
      char *field = malloc (len + 1);
      if (field == NULL)
	return ENOMEM;
      memcpy (field, name, len);
      field[len] = '\0';
      thdr[header->fhdr_count].fn = field;
      thdr[header->fhdr_count].fn_end = field + len;

      if (buffer)
	{
          len = strlen (buffer);
          field =  malloc (len + 1);
          if (field == NULL)
	    return ENOMEM;
	  memcpy (field, buffer, len);
	  field[len] = '\0';
	  thdr[header->fhdr_count].fv = field;
	  thdr[header->fhdr_count].fv_end = field + len;
	}
      else
	{
	  thdr[header->fhdr_count].fv = NULL;
	  thdr[header->fhdr_count].fv_end = NULL;
	}
      header->fhdr_count++;
      header->fhdr = thdr;
      return 0;
    }
  return ENOMEM;
}

/* For the cache header if the field exist but with no corresponding
   value, it is a permanent failure i.e.  the field does not exist
   in the header return EINVAL to notify mu_header_get_value().  */
static int
header_get_fvalue (mu_header_t header, const char *name, char *buffer,
		  size_t buflen, size_t *pn)
{
  size_t i, fn_len, fv_len = 0;
  size_t name_len;
  int err = MU_ERR_NOENT;

  for (i = 0, name_len = strlen (name); i < header->fhdr_count; i++)
    {
      fn_len = header->fhdr[i].fn_end - header->fhdr[i].fn;
      if (fn_len == name_len
	  && strcasecmp (header->fhdr[i].fn, name) == 0)
	{
	  fv_len = header->fhdr[i].fv_end - header->fhdr[i].fv;

	  /* Permanent failure.  */
	  if (fv_len == 0)
	    {
	      err = EINVAL;
	      break;
	    }

	  if (buffer && buflen > 0)
	    {
	      buflen--;
	      fv_len = (fv_len < buflen) ? fv_len : buflen;
	      memcpy (buffer, header->fhdr[i].fv, fv_len);
	      buffer[fv_len] = '\0';
	    }
	  err = 0;
	  break;
	}
    }
  if (pn)
    *pn = fv_len;
  return err;

}

int
mu_header_get_value (mu_header_t header, const char *name, char *buffer,
		  size_t buflen, size_t *pn)
{
  size_t i = 0;
  size_t name_len;
  size_t total = 0, fn_len = 0, fv_len = 0;
  size_t threshold;
  int err = 0;

  if (header == NULL || name == NULL)
    return EINVAL;

  /* First scan our cache headers for hits.  */
  err = header_get_fvalue (header, name, buffer, buflen, pn);
  switch (err)
    {
    case EINVAL: /* Permanent failure.  */
      err = MU_ERR_NOENT;
    case ENOMEM:
      if (pn)
	*pn = 0;
    case 0:
      return err;
    }

  /* Try the provided cache.  */
  if (header->_get_fvalue)
    err = header->_get_fvalue (header, name, buffer, buflen, pn);
  if (err == 0)
    return 0;

  if (header->_get_value)
    {
      char buf[1024]; /* should suffice for field-value. */
      size_t len = 0;
      err = header->_get_value (header, name, buf, sizeof (buf), &len);
      if (err == 0)
	{
	  /* Save in the fast header buffer.  */
	  header_set_fvalue (header, name, buf);
	  if (buffer && buflen > 0)
	    {
	      buflen--;
	      buflen = (len < buflen) ? len : buflen;
	      memcpy (buffer, buf, buflen);
	      buffer[buflen] = '\0';
	    }
	  else
	    buflen = len;
	  if (pn)
	    *pn = buflen;
	}
      else
        {
	  /* Cache permanent failure also.  */
	  header_set_fvalue (header, name, NULL);
        }
      return err;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  /* We set the threshold to be 1 less for the null.  */
  threshold = --buflen;

  /* Caution: We may have more then one value for a field name, for example
     a "Received" field-name is added by each passing MTA.  The way that the
     parsing (_parse()) is done it's not take to account.  So we just stuff
     in the buffer all the field-values to a corresponding field-name.
     FIXME: Should we kosher the output ? meaning replace occurences of
     " \t\r\n" for spaces ? for now we don't.
   */
  for (name_len = strlen (name), i = 0; i < header->hdr_count; i++)
    {
      fn_len = header->hdr[i].fn_end - header->hdr[i].fn;
      if (fn_len == name_len &&
	  strncasecmp (header->hdr[i].fn, name, fn_len) == 0)
	{
	  fv_len = (header->hdr[i].fv_end - header->hdr[i].fv);
	  /* FIXME:FIXME:PLEASE: hack, add a space/nl separator  */
	  /*
	  if (total && (threshold - 2) > 0)
	    {
	      if (buffer)
		{
		  *buffer++ = '\n';
		  *buffer++ = ' ';
		}
	      threshold -= 2;
	      total += 2;
	    }
          */
	  total += fv_len;
	  /* Can everything fit in the buffer.  */
	  if (buffer && threshold > 0)
	    {
	      buflen = (fv_len < threshold) ? fv_len : threshold;
	      memcpy (buffer, header->hdr[i].fv, buflen);
	      buffer += buflen;
	      threshold -= buflen;
	    }

	  /* Jump out after the first header we found. -sr */
	  break;
	}
    }
  if (buffer)
    *buffer = '\0'; /* Null terminated.  */
  if (pn)
    *pn = total;

  return  (total == 0) ? MU_ERR_NOENT : 0;
}

int
mu_header_get_value_unfold (mu_header_t header, const char *name, char *buffer,
			 size_t buflen, size_t *pn)
{
  int rc = mu_header_get_value (header, name, buffer, buflen, pn);

  if (rc == 0)
    mu_string_unfold (buffer, pn);
  return rc;
}

int
mu_header_aget_value (mu_header_t header, const char *name, char **pvalue)
{
  char *value;
  size_t n = 0;
  int status = mu_header_get_value (header, name, NULL, 0, &n);
  if (status == 0)
    {
      value = calloc (n + 1, 1);
      if (value == NULL)
        return ENOMEM;
      mu_header_get_value (header, name, value, n + 1, NULL);
      *pvalue = value;
    }

  return status;
}

int
mu_header_aget_value_unfold (mu_header_t header, const char *name, char **pvalue)
{
  int rc = mu_header_aget_value (header, name, pvalue);
  if (rc == 0)
    mu_string_unfold (*pvalue, NULL);
  return rc;
}

int
mu_header_get_address (mu_header_t header, const char *name, mu_address_t *addr)
{
  char* value = NULL;
  int status = mu_header_aget_value(header, name, &value);

  if(status)
    return status;

  status = mu_address_create(addr, value);

  free(value);

  return status;
}

int
mu_header_get_field_count (mu_header_t header, size_t *pcount)
{
  if (header == NULL)
    {
      if (pcount)
        *pcount = 0;
      return EINVAL;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (pcount)
    *pcount = header->hdr_count;
  return 0;
}

int
mu_header_get_field_name (mu_header_t header, size_t num, char *buf,
		       size_t buflen, size_t *nwriten)
{
  size_t len;

  if (header == NULL)
    return EINVAL;

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (header->hdr_count == 0 || num > header->hdr_count || num == 0)
    return MU_ERR_NOENT;

  num--;
  len = (header->hdr[num].fn_end - header->hdr[num].fn);
  if (buf && buflen)
    {
      /* save one for the null */
      --buflen;
      len = (len > buflen) ? buflen : len;
      memcpy (buf, header->hdr[num].fn, len);
      buf[len] = '\0';
    }
  if (nwriten)
    *nwriten = len;
  return 0;
}

int
mu_header_aget_field_name (mu_header_t header, size_t num, char **pvalue)
{
  char *value;
  size_t n = 0;
  int status = mu_header_get_field_name (header, num, NULL, 0, &n);
  if (status == 0)
    {
      value = calloc (n + 1, 1);
      if (value == NULL)
        return ENOMEM;
      mu_header_get_field_name (header, num, value, n + 1, NULL);
      *pvalue = value;
    }
  else
    *pvalue  = strdup ("");
  return status;
}

int
mu_header_get_field_value (mu_header_t header, size_t num, char *buf,
			size_t buflen, size_t *nwritten)
{
  size_t len;

  if (header == NULL || num < 1)
    return EINVAL;

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (header->hdr_count == 0 || num > header->hdr_count || num == 0)
    return MU_ERR_NOENT;

  num--;
  len = header->hdr[num].fv_end - header->hdr[num].fv;
  if (buf && buflen > 0)
    {
      /* save one for the null */
      --buflen;
      len = (len > buflen) ? buflen : len;
      memcpy (buf, header->hdr[num].fv, len);
      buf[len] = '\0';
    }

  if (nwritten)
    *nwritten = len;
  return 0;
}

int
mu_header_get_field_value_unfold (mu_header_t header, size_t num, char *buf,
			       size_t buflen, size_t *nwritten)
{
  int rc = mu_header_get_field_value (header, num, buf, buflen, nwritten);
  if (rc == 0)
    mu_string_unfold (buf, nwritten);
  return rc;
}

int
mu_header_aget_field_value (mu_header_t header, size_t num, char **pvalue)
{
  char *value;
  size_t n = 0;
  int status = mu_header_get_field_value (header, num, NULL, 0, &n);
  if (status == 0)
    {
      value = calloc (n + 1, 1);
      if (value == NULL)
        return ENOMEM;
      mu_header_get_field_value (header, num, value, n + 1, NULL);
      *pvalue = value;
    }
  else
    *pvalue = strdup ("");
  return status;
}

int
mu_header_aget_field_value_unfold (mu_header_t header, size_t num, char **pvalue)
{
  int rc = mu_header_aget_field_value (header, num, pvalue);
  if (rc == 0)
    mu_string_unfold (*pvalue, NULL);
  return rc;
}

int
mu_header_set_lines (mu_header_t header, int (*_lines)
		 (mu_header_t, size_t *), void *owner)
{
  if (header ==  NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_lines = _lines;
  return 0;
}

int
mu_header_lines (mu_header_t header, size_t *plines)
{
  int n;
  size_t lines = 0;

  if (header == NULL)
    return EINVAL;
  if (plines == NULL)
    return MU_ERR_OUT_PTR_NULL;

  /* Overload.  */
  if (header->_lines)
    return header->_lines (header, plines);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  for (n = header->blurb_len - 1; n >= 0; n--)
    {
      if (header->blurb[n] == '\n')
	lines++;
    }
  if (plines)
    *plines = lines;
  return 0;
}

int
mu_header_set_size (mu_header_t header, int (*_size)
		 (mu_header_t, size_t *), void *owner)
{
  if (header ==  NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_size = _size;
  return 0;
}

int
mu_header_size (mu_header_t header, size_t *psize)
{
  if (header == NULL)
      return EINVAL;

  /* Overload.  */
  if (header->_size)
    return header->_size (header, psize);

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  if (psize)
    *psize = header->blurb_len;
  return 0;
}

int
mu_header_set_get_fvalue (mu_header_t header, 
       int (*_get_fvalue) (mu_header_t, const char *, char *, size_t, size_t *), 
                       void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_get_fvalue = _get_fvalue;
  return 0;
}

int
mu_header_set_get_value (mu_header_t header, int (*_get_value)
		     (mu_header_t, const char *, char *, size_t, size_t *),
		     void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_get_value = _get_value;
  return 0;
}

int
mu_header_set_set_value (mu_header_t header, int (*_set_value)
		      (mu_header_t , const char *, const char *, int),
		      void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_set_value = _set_value;
  return 0;
}

int
mu_header_set_stream (mu_header_t header, mu_stream_t stream, void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->stream = stream;
  return 0;
}

int
mu_header_set_fill (mu_header_t header, int
		 (*_fill) (mu_header_t, char *, size_t, mu_off_t, size_t *),
		 void *owner)
{
  if (header == NULL)
    return EINVAL;
  if (header->owner != owner)
    return EACCES;
  header->_fill = _fill;
  return 0;
}

static int
fill_blurb (mu_header_t header)
{
  int status;
  char buf[1024];
  size_t nread;

  if (header->_fill == NULL)
    return 0;

  /* The entire header is now ours(part of mu_header_t), clear all the
     overloading.  */
  header_free_cache (header);
  header->_get_fvalue = NULL;
  header->_get_value = NULL;
  header->_set_value = NULL;
  header->_size = NULL;
  header->_lines = NULL;

  if (header->mstream == NULL)
    {
      status = mu_memory_stream_create (&header->mstream, NULL, MU_STREAM_RDWR);
      if (status != 0)
	return status;
      mu_stream_open (header->mstream);
      header->stream_len = 0;
    }

  /* Bring in the entire header.  */
  do
    {
      nread = 0;
      status = header->_fill (header, buf, sizeof buf,
			      header->stream_len, &nread) ;
      if (status != 0)
	{
	  if (status != EAGAIN && status != EINTR)
	    {
	      mu_stream_destroy (&(header->mstream), NULL);
	      header->stream_len = 0;
	    }
	  return status;
	}
      if (nread > 0)
	{
	  status = mu_stream_write (header->mstream, buf, nread, header->stream_len, NULL);
	  if (status != 0)
	    {
	      mu_stream_destroy (&(header->mstream), NULL);
	      header->stream_len = 0;
	      return status;
	    }
	  header->stream_len += nread;
	}
    }
  while (nread > 0);

  /* parse it. */
  {
    char *blurb;
    size_t len = header->stream_len;
    blurb = calloc (1, len + 1);
    if (blurb)
      {
	mu_stream_read (header->mstream, blurb, len, 0, &len);
	status = header_parse (header, blurb, len);
      }
    free (blurb);
  }
  mu_stream_destroy (&header->mstream, NULL);
  header->stream_len = 0;
  return status;
}

static int
header_write (mu_stream_t os, const char *buf, size_t buflen,
	      mu_off_t off, size_t *pnwrite)
{
  mu_header_t header = mu_stream_get_owner (os);
  int status;

  if (header == NULL)
    return EINVAL;

  if ((size_t)off != header->stream_len)
    return ESPIPE;

  /* Skip the obvious.  */
  if (buf == NULL || *buf == '\0' || buflen == 0)
    {
      if (pnwrite)
        *pnwrite = 0;
      return 0;
    }

  if (header->mstream == NULL)
    {
      status = mu_memory_stream_create (&header->mstream, NULL, MU_STREAM_RDWR);
      if (status != 0)
	return status;
      status = mu_stream_open (header->mstream);
      if (status != 0)
      {
	mu_stream_destroy(&header->mstream, NULL);
	return status;
      }
      header->stream_len = 0;
    }

  status = mu_stream_write (header->mstream, buf, buflen, header->stream_len, &buflen);
  if (status != 0)
    {
      mu_stream_destroy (&header->mstream, NULL);
      header->stream_len = 0;
      return status;
    }
  header->stream_len += buflen;

  /* We detect an empty line .i.e "^\n$" this signal the end of the
     header.  */
  if (header->stream_len)
    {
      int finish = 0;
      char nlnl[2];
      nlnl[1] = nlnl[0] = '\0';
      mu_stream_read (header->mstream, nlnl, 1, 0, NULL);
      if (nlnl[0] == '\n')
	{
	  finish = 1;
	}
      else
	{
	  mu_stream_read (header->mstream, nlnl, 2, header->stream_len - 2, NULL);
	  if (nlnl[0] == '\n' && nlnl[1] == '\n')
	    {
	      finish = 1;
	    }
	}
      if (finish)
	{
	  char *blurb;
	  size_t len = header->stream_len;
	  blurb = calloc (1, len + 1);
	  if (blurb)
	    {
	      mu_stream_read (header->mstream, blurb, len, 0, &len);
	      status = header_parse (header, blurb, len);
	    }
	  free (blurb);
	  mu_stream_destroy (&header->mstream, NULL);
	  header->stream_len = 0;
	}
  }

  if (pnwrite)
    *pnwrite = buflen;
  return 0;

}

static int
header_read (mu_stream_t is, char *buf, size_t buflen, mu_off_t off, size_t *pnread)
{
  int len;
  mu_header_t header = mu_stream_get_owner (is);

  if (is == NULL || header == NULL)
    return EINVAL;

  if (buf == NULL || buflen == 0)
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  len = header->blurb_len - off;
  if (len > 0)
    {
      len = (buflen < (size_t)len) ? buflen : (size_t)len;
      memcpy (buf, header->blurb + off, len);
    }
  else
    len = 0;

  if (pnread)
    *pnread = len;
  return 0;
}

static int
header_readline (mu_stream_t is, char *buf, size_t buflen, mu_off_t off, size_t *pn)
{
  int len;
  mu_header_t header = mu_stream_get_owner (is);

  if (is == NULL || header == NULL)
    return EINVAL;

  if (buf == NULL || buflen == 0)
    {
      if (pn)
	*pn = 0;
      return 0;
    }

  /* Try to fill out the buffer, if we know how.  */
  if (header->blurb == NULL)
    {
      int err = fill_blurb (header);
      if (err != 0)
	return err;
    }

  buflen--; /* Space for the null.  */

  len = header->blurb_len - off;
  if (len > 0)
    {
      char *nl = memchr (header->blurb + off, '\n', len);
      if (nl)
	len = nl - (header->blurb + off) + 1;
      len = (buflen < (size_t)len) ? buflen : (size_t)len;
      memcpy (buf, header->blurb + off, len);
    }
  else
    len = 0;
  if (pn)
    *pn = len;
  buf[len] = '\0';
  return 0;
}

int
mu_header_get_stream (mu_header_t header, mu_stream_t *pstream)
{
  if (header == NULL)
    return EINVAL;
  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (header->stream == NULL)
    {
      int status = mu_stream_create (&(header->stream), MU_STREAM_RDWR, header);
      if (status != 0)
	return status;
      mu_stream_set_read (header->stream, header_read, header);
      mu_stream_set_readline (header->stream, header_readline, header);
      mu_stream_set_write (header->stream, header_write, header);
    }
  *pstream = header->stream;
  return 0;
}

