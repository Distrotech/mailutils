/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007-2012 Free Software
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

/*  This all header business needs a good rewrite.
 *          -- Alain Magloire, 2000-07-03 (rev. 1.21)
 *
 *  It's the job that's never started as takes longest to finish.
 *          -- Hamfast Gamgee, some time in the Third Age
 *
 *  It took almost 7 years to gather the courage to start the job,
 *  and only one day to finish it.
 *          -- Sergey Poznyakoff, 2007-06-24
 */

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
#include <mailutils/util.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>
#include <mailutils/sys/header_stream.h>
#include <mailutils/sys/header.h>

#define HEADER_MODIFIED   0x01
#define HEADER_INVALIDATE 0x02
#define HEADER_STREAMMOD  0x04

#define HEADER_SET_MODIFIED(h) \
  ((h)->flags |= (HEADER_MODIFIED|HEADER_INVALIDATE))


/* mu_hdrent manipulation */

#define MU_HDRENT_NAME(hp,ep) ((hp)->spool + (ep)->fn)
#define MU_HDRENT_VALUE(hp,ep) ((hp)->spool + (ep)->fv)
#define MU_STR_SIZE(nlen,vlen) ((nlen) + 2 + (vlen) + 1)

static struct mu_hdrent *
mu_hdrent_nth (struct _mu_header *hdr, int n)
{
  struct mu_hdrent *p;
  for (p = hdr->head; p; p = p->next)
    if (n-- == 1)
      break;
  return p;
}

static struct mu_hdrent *
mu_hdrent_find (struct _mu_header *hdr, const char *name, int pos)
{
  struct mu_hdrent *p;

  if (pos > 0)
    {
      for (p = hdr->head; p; p = p->next)
	if ((!name || mu_c_strcasecmp (MU_HDRENT_NAME (hdr,p), name) == 0) &&
	    pos-- == 1)
	  break;
    }
  else if (pos < 0)
    {
      for (p = hdr->tail; p; p = p->prev)
	if ((!name || mu_c_strcasecmp (MU_HDRENT_NAME (hdr,p), name) == 0) &&
	    ++pos == 0)
	  break;
    }
  else
    p = NULL;
  return p;
}

static int
mu_hdrent_find_stream_pos (struct _mu_header *hdr, mu_off_t pos,
			   struct mu_hdrent **pent, size_t *poff)
{
  mu_off_t x;
  struct mu_hdrent *p;

  for (p = hdr->head, x = 0; p; p = p->next)
    {
      size_t strsize = MU_STR_SIZE (p->nlen, p->vlen);
      if (x <= pos && pos < x + strsize)
	{
	  *poff = pos - x;
	  *pent = p;
	  return 0;
	}
      x += strsize;
    }
  if (x == pos && hdr->tail)
    {
      /* To supply the trailing '\n' */
      p = hdr->tail;
      *pent = p;
      *poff = MU_STR_SIZE (p->nlen, p->vlen) - 1;
      return 0;
    }
  return 1;
}
     
static void
mu_hdrent_count (struct _mu_header *hdr, size_t *pcount, size_t *psize,
		 size_t *plines)
{
  if (hdr->flags & HEADER_INVALIDATE)
    {
      size_t size = 0;
      size_t count = 0;
      size_t lines = 0;
      struct mu_hdrent *p;
      for (p = hdr->head; p; p = p->next)
	{
	  count++;
	  size += MU_STR_SIZE (p->nlen, p->vlen);
	  lines += p->nlines;
	}

      hdr->numhdr = count;
      hdr->numlines = lines;
      hdr->size = size;
      hdr->flags &= ~HEADER_INVALIDATE;
    }
      
  *pcount = hdr->numhdr;
  *psize = hdr->size;
  *plines = hdr->numlines;
}

static void
mu_hdrent_remove (struct _mu_header *hdr, struct mu_hdrent *ent)
{
  struct mu_hdrent *p = ent->prev;
  if (p)
    p->next = ent->next;
  else
    hdr->head = ent->next;

  p = ent->next;
  if (p)
    p->prev = ent->prev;
  else
    hdr->tail = ent->prev;
}

static void
mu_hdrent_prepend (struct _mu_header *hdr, struct mu_hdrent *ent)
{
  struct mu_hdrent *p = hdr->head;
  ent->prev = NULL;
  ent->next = p;
  if (p)
    p->prev = ent;
  else
    hdr->tail = ent;
  hdr->head = ent;
}

static void
mu_hdrent_append (struct _mu_header *hdr, struct mu_hdrent *ent)
{
  struct mu_hdrent *p = hdr->tail;
  ent->next = NULL;
  ent->prev = p;
  if (p)
    p->next = ent;
  else
    hdr->head = ent;
  hdr->tail = ent;
}

static int
mu_hdrent_insert (struct _mu_header *hdr, struct mu_hdrent *ent,
		  const char *name, int pos, int before)
{
  struct mu_hdrent *p;
  struct mu_hdrent *ref = mu_hdrent_find (hdr, name, pos);
  if (!ref)
    return MU_ERR_NOENT;

  if (before)
    {
      ref = ref->prev;
      if (!ref)
	{
	  mu_hdrent_prepend (hdr, ent);
	  return 0;
	}
    }

  p = ref->next;
  if (!p)
    {
      mu_hdrent_append (hdr, ent);
      return 0;
    }

  ent->next = p;
  p->prev = ent;
  ent->prev = ref;
  ref->next = ent;
  
  return 0;
}

#define SPOOLBLKSIZ 1024

static struct mu_hdrent *
mu_hdrent_create (struct _mu_header *ph,
		  struct mu_hdrent *ent,
		  const char *name, size_t nsize,
		  const char *value, size_t vsize)
{
  size_t strsize;
  size_t sizeleft;
  const char *p;
  
  if (!ent)
    {
      ent = calloc (1, sizeof (*ent));
      if (!ent)
	return NULL;
    }
  
  strsize = MU_STR_SIZE (nsize, vsize);
  sizeleft = ph->spool_size - ph->spool_used;

  /* Ensure there is enough space in spool */
  if (sizeleft < strsize)
    {
      char *newp;
      size_t delta = (strsize - sizeleft + SPOOLBLKSIZ - 1) / SPOOLBLKSIZ;
      delta *= SPOOLBLKSIZ;
      newp = realloc (ph->spool, ph->spool_size + delta);
      if (!newp)
	return 0;
      ph->spool = newp;
      ph->spool_size += delta;
    }

  /* Copy header name */
  ent->fn = ph->spool_used;
  ent->nlen = nsize;
  memcpy (ph->spool + ph->spool_used, name, nsize);
  ph->spool_used += nsize;
  ph->spool[ph->spool_used++] = 0;
  ph->spool[ph->spool_used++] = ' ';

  /* Copy header value */
  ent->fv = ph->spool_used;
  ent->vlen = vsize;
  memcpy (ph->spool + ph->spool_used, value, vsize);
  ph->spool_used += vsize;
  ph->spool[ph->spool_used++] = 0;
  
  ent->nlines = 1;
  for (p = value; p < value + vsize; p++)
    if (*p == '\n')
      ent->nlines++;
  
  return ent;
}

static void
mu_hdrent_free_list (struct _mu_header *hdr)
{
  struct mu_hdrent *p;
  for (p = hdr->head; p; )
    {
      struct mu_hdrent *next = p->next;
      free (p);
      p = next;
    }
  hdr->head = hdr->tail = NULL;
  hdr->spool_used = 0;
}
  


#define ISLWSP(c) (((c) == ' ' || (c) == '\t'))

/* Parsing is done in a rather simple fashion, meaning we just consider an
   entry to be a field-name an a field-value.  So they maybe duplicate of
   field-name like "Received" they are just put in the array, see _get_value()
   on how to handle the case. in the case of error .i.e a bad header construct
   we do a full stop and return what we have so far.  */

static int
header_parse (mu_header_t header, const char *blurb, int len)
{
  const char *header_end;
  const char *header_start;
  const char *header_start2;
  
  /* Nothing to parse.  */
  if (blurb == NULL)
    return 0;

  header->flags |= HEADER_INVALIDATE;
  mu_hdrent_free_list (header);

  /* Get a header, a header is:
     field-name LWSP ':'
       LWSP field-value '\r' '\n'
       *[ (' ' | '\t') field-value '\r' '\n' ]
  */
  /* First loop goes through the blurb */
  for (header_start = blurb; len > 0; header_start = ++header_end)
    {
      const char *fn, *fn_end, *fv, *fv_end;
      struct mu_hdrent *ent;

      if (header_start[0] == ' '
	  || header_start[0] == '\t'
	  || header_start[0] == '\n')
	break;

      /* Second loop extract one header field. */
      for (header_start2 = header_start; len; header_start2 = ++header_end)
	{
	  header_end = memchr (header_start2, '\n', len);
	  if (header_end == NULL)
	    {
	      header_end = header_start2 + len;
	      len = 0;
	      break;
	    }
	  else
	    {
	      len -= (header_end - header_start2 + 1);
	      if (!len
		  || (header_end[1] != ' '
		      && header_end[1] != '\t'))
		break; /* New header break the inner for. */
	    }
	  /* *header_end = ' ';  smash LF ? NO */
	}

      /* Now save the header in the data structure.  */

      /* Treats unix "From " specially.  FIXME: Should we? */
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

	  /* Houston we have a problem.  */
	  if (colon == NULL)
	    break; /* FIXME: Disregard the rest and bailout.  */

	  fn = header_start;
	  fn_end = colon;
	  /* Shrink any LWSP after the field name -- CRITICAL for 
	     later name comparisons to work correctly! */
	  while (ISLWSP (fn_end[-1]))
	    fn_end--;

	  fv = colon + 1;
	  fv_end = header_end;

	  /* Skip any LWSP before the field value -- unnecessary, but
	     might make some field values look a little tidier. */
	  while (ISLWSP (fv[0]))
	    fv++;
	}

      /* Register this header */
      ent = mu_hdrent_create (header, NULL, fn, fn_end - fn, fv, fv_end - fv);
      if (!ent)
	return ENOMEM;
      mu_hdrent_append (header, ent);
    } /* for (header_start ...) */

  return 0;
}


static int 
mu_header_fill (mu_header_t header)
{
  int status;
  size_t blurb_len = 0;
  char *blurb = NULL;
  
  if (header->mstream && header->flags & HEADER_STREAMMOD)
    {
      mu_off_t end;

      mu_header_invalidate (header);
      status = mu_stream_size (header->mstream, &end);
      if (status)
	return status;
      status = mu_stream_seek (header->mstream, 0, MU_SEEK_SET, NULL);
      if (status)
	return status;      
      blurb_len = end;
      blurb = malloc (blurb_len + 1);
      if (!blurb)
	return ENOMEM;
      status = mu_stream_read (header->mstream, blurb, blurb_len, NULL);
      if (status)
	{
	  free (blurb);
	  return status;
	}
      status = header_parse (header, blurb, blurb_len);
      free (blurb);
      if (status == 0)
	header->flags &= ~HEADER_STREAMMOD;
      return status;
    }
  if (header->spool_used)
    return 0;
  
  if (header->_fill == NULL)
    return 0; /* FIXME: Really? */

  /* Bring in the entire header.  */
  status = header->_fill (header->data, &blurb, &blurb_len);
  if (status)
    return status;
  status = header_parse (header, blurb, blurb_len);
  free (blurb);
  return status;
}



int
mu_header_create (mu_header_t *ph, const char *blurb, size_t len)
{
  mu_header_t header;
  int status = 0;

  header = calloc (1, sizeof (*header));
  if (header == NULL)
    return ENOMEM;
  
  status = header_parse (header, blurb, len);

  *ph = header;
  return status;
}

void
mu_header_destroy (mu_header_t *ph)
{
  if (ph && *ph)
    {  
      mu_header_t header = *ph;

      mu_stream_destroy (&header->mstream);
      mu_stream_destroy (&header->stream);
      mu_hdrent_free_list (header);
      free (header->spool);
      free (header);
      *ph = NULL;
    }
}


int
mu_header_set_value (mu_header_t header, const char *fn, const char *fv,
		     int replace)
{
  int status;
  struct mu_hdrent *ent = NULL;
  
  if (header == NULL || fn == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status)
    return status;
  
  /* An fv of NULL means delete the field, but only do it if replace
     was also set to true! */
  if (fv == NULL && !replace)
    return EINVAL;

  if (replace)
    {
      ent = mu_hdrent_find (header, fn, 1);
      if (ent)
	{
	  if (fv == NULL)
	    {
	      /* Delete the header */
	      mu_hdrent_remove (header, ent);
	      free (ent);
	      return 0;
	    }
	  mu_hdrent_create (header, ent, fn, strlen (fn), fv, strlen (fv));
	  HEADER_SET_MODIFIED (header);
	  return 0;
	}
      else if (fv == NULL)
	return 0;
    }

  ent = mu_hdrent_create (header, NULL,
			  fn, strlen (fn), fv, strlen (fv));
  if (!ent)
    return ENOMEM;
  mu_hdrent_prepend (header, ent);
  HEADER_SET_MODIFIED (header);
  return 0;
}

int
mu_header_remove (mu_header_t header, const char *fn, int n)
{
  int status;
  struct mu_hdrent *ent;
  
  if (header == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status)
    return status;

  ent = mu_hdrent_find (header, fn, n);
  if (!ent)
    return MU_ERR_NOENT;

  mu_iterator_delitem (header->itr, ent);
  mu_hdrent_remove (header, ent);
  HEADER_SET_MODIFIED (header);
  free (ent);
  return 0;
}

int
mu_header_append (mu_header_t header, const char *fn, const char *fv)
{
  int status;
  struct mu_hdrent *ent;

  if (header == NULL || fn == NULL || fv == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status)
    return status;

  ent = mu_hdrent_create (header, NULL, fn, strlen (fn), fv, strlen (fv));
  if (!ent)
    return ENOMEM;
  mu_hdrent_append (header, ent);
  HEADER_SET_MODIFIED (header);
  return 0;
}

int
mu_header_prepend (mu_header_t header, const char *fn, const char *fv)
{
  int status;
  struct mu_hdrent *ent;

  if (header == NULL || fn == NULL || fv == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status)
    return status;

  ent = mu_hdrent_create (header, NULL, fn, strlen (fn), fv, strlen (fv));
  if (!ent)
    return ENOMEM;
  mu_hdrent_prepend (header, ent);
  HEADER_SET_MODIFIED (header);
  return 0;
}

int
mu_header_insert (mu_header_t header,
		  const char *fn, const char *fv, 
		  const char *ref, int n, int flags)
{
  int status;
  struct mu_hdrent *ent;

  if (header == NULL || fn == NULL || fv == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status)
    return status;
    
  if (flags & MU_HEADER_REPLACE)
    {
      if (!ref)
	ref = fn;
      ent = mu_hdrent_find (header, ref, n);
      mu_hdrent_create (header, ent, fn, strlen (fn), fv, strlen (fv));
    }
  else
    {
      ent = mu_hdrent_create (header, NULL,
			      fn, strlen (fn), fv, strlen (fv));
      if (!ent)
	return ENOMEM;
      if (ref)
	return mu_hdrent_insert (header, ent, ref, n,
				 flags & MU_HEADER_BEFORE);
      else
	mu_hdrent_prepend (header, ent);
    }
  HEADER_SET_MODIFIED (header);
  return 0;
}

int
mu_header_clear (mu_header_t header)
{
  int status;
  
  if (header == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status)
    return status;
  mu_header_invalidate (header);
  HEADER_SET_MODIFIED (header);
  return 0;
}


int
mu_header_sget_value_n (mu_header_t header,
			const char *name, int n,
			const char **pval)
{
  int status;
  struct mu_hdrent *ent;

  if (header == NULL || name == NULL)
    return EINVAL;
  status = mu_header_fill (header);
  if (status)
    return status;

  ent = mu_hdrent_find (header, name, n);
  if (!ent)
    return MU_ERR_NOENT;
  if (pval)
    *pval = MU_HDRENT_VALUE (header, ent);
  return 0;
}

int
mu_header_aget_value_n (mu_header_t header,
		      const char *name, int n,
		      char **pval)
{
  const char *s;
  int status = mu_header_sget_value_n (header, name, n, &s);
  if (status == 0)
    {
      *pval = strdup (s);
      if (!*pval)
	status = ENOMEM;
    }
  return status;
}

int
mu_header_get_value_n (mu_header_t header, const char *name, int n,
		       char *buffer, size_t buflen, size_t *pn)
{
  const char *s;
  int status = mu_header_sget_value_n (header, name, n, &s);
  if (status == 0)
    {
      size_t slen = strlen (s);

      if (buffer)
	{
	  if (slen > buflen)
	    slen = buflen;
	  memcpy (buffer, s, slen);
	  buffer[slen] = 0;
	}
      if (pn)
	*pn = slen;
    }
  return status;
}


/* Unfolding functions */
int
mu_header_get_value_unfold_n (mu_header_t header,
			      const char *name, int n, char *buffer,
			      size_t buflen, size_t *pn)
{
  int rc = mu_header_get_value_n (header, name, n, buffer, buflen, pn);

  if (rc == 0)
    mu_string_unfold (buffer, pn);
  return rc;
}

int
mu_header_aget_value_unfold_n (mu_header_t header, const char *name, int n,
			       char **pvalue)
{
  int rc = mu_header_aget_value_n (header, name, n, pvalue);
  if (rc == 0)
    mu_string_unfold (*pvalue, NULL);
  return rc;
}


int
mu_header_get_address_n (mu_header_t header, const char *name, int n,
			 mu_address_t *addr)
{
  const char *value = NULL;
  int status = mu_header_sget_value_n (header, name, n, &value);

  if (status)
    return status;

  return mu_address_create (addr, value);
}


int
mu_header_get_field_count (mu_header_t header, size_t *pcount)
{
  size_t count;
  size_t size;
  size_t lines;
  int status;
  
  if (header == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status == 0)
    {
      mu_hdrent_count (header, &count, &size, &lines);
      
      if (pcount)
	*pcount = count;
    }
  
  return status;
}

int
mu_header_get_itemptr (mu_header_t header, size_t num, const void **sptr)
{
  int status;
  
  if (header == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status == 0)
    {
      struct mu_hdrent *ent = mu_hdrent_nth (header, num);
      if (ent)
	*sptr = ent;
      else
	status = MU_ERR_NOENT;
    }
  return status;
}

int
mu_header_sget_field_name (mu_header_t header, size_t num, const char **sptr)
{
  int status;
  
  if (header == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status == 0)
    {
      struct mu_hdrent *ent = mu_hdrent_nth (header, num);
      if (ent)
	*sptr = MU_HDRENT_NAME (header, ent);
      else
	status = MU_ERR_NOENT;
    }
  return status;
}

int
mu_header_get_field_name (mu_header_t header, size_t num, char *buffer,
			  size_t buflen, size_t *pn)
{
  const char *s;
  int status = mu_header_sget_field_name (header, num, &s);
  if (status == 0)
    {
      size_t slen = strlen (s);

      if (buffer)
	{
	  if (slen > buflen)
	    slen = buflen;
	  memcpy (buffer, s, slen);
	  buffer[slen] = 0;
	}
      if (pn)
	*pn = slen;
    }
  return status;
}

int
mu_header_aget_field_name (mu_header_t header, size_t num, char **pvalue)
{
  const char *s;
  int status = mu_header_sget_field_name (header, num, &s);
  if (status == 0)
    {
      if ((*pvalue = strdup (s)) == NULL)
	status = ENOMEM;
    }
  return status;
}


int
mu_header_sget_field_value (mu_header_t header, size_t num, const char **sptr)
{
  int status;
  
  if (header == NULL)
    return EINVAL;

  status = mu_header_fill (header);
  if (status == 0)
    {
      struct mu_hdrent *ent = mu_hdrent_nth (header, num);
      if (ent)
	*sptr = MU_HDRENT_VALUE (header, ent);
      else
	status = MU_ERR_NOENT;
    }
  return status;
}

int
mu_header_get_field_value (mu_header_t header, size_t num, char *buffer,
			   size_t buflen, size_t *pn)
{
  const char *s;
  int status = mu_header_sget_field_value (header, num, &s);
  if (status == 0)
    {
      size_t slen = strlen (s);

      if (buffer)
	{
	  if (slen > buflen)
	    slen = buflen;
	  memcpy (buffer, s, slen);
	  buffer[slen] = 0;
	}
      if (pn)
	*pn = slen;
    }
  return status;
}

int
mu_header_aget_field_value (mu_header_t header, size_t num, char **pvalue)
{
  const char *s;
  int status = mu_header_sget_field_value (header, num, &s);
  if (status == 0)
    {
      if ((*pvalue = strdup (s)) == NULL)
	status = ENOMEM;
    }
  return status;
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
mu_header_aget_field_value_unfold (mu_header_t header, size_t num,
				   char **pvalue)
{
  int rc = mu_header_aget_field_value (header, num, pvalue);
  if (rc == 0)
    mu_string_unfold (*pvalue, NULL);
  return rc;
}


int
mu_header_lines (mu_header_t header, size_t *plines)
{
  int status;

  if (header == NULL)
    return EINVAL;
  if (plines == NULL)
    return MU_ERR_OUT_PTR_NULL;

  status = mu_header_fill (header);
  if (status == 0)
    {
      size_t count;
      size_t size;
      size_t lines;
      mu_hdrent_count (header, &count, &size, &lines);
      *plines = lines + 1;
    }
  return status;
}

int
mu_header_size (mu_header_t header, size_t *psize)
{
  int status;

  if (header == NULL)
    return EINVAL;
  if (psize == NULL)
    return MU_ERR_OUT_PTR_NULL;

  status = mu_header_fill (header);
  if (status == 0)
    {
      size_t count;
      size_t size;
      size_t lines;
      mu_hdrent_count (header, &count, &size, &lines);
      *psize = size + 1;
    }
  return status;
}

int
mu_header_invalidate (mu_header_t hdr)
{
  if (hdr == NULL)
    return EINVAL;
  mu_hdrent_free_list (hdr);
  return 0;
}


static void
mu_hdrent_fixup (mu_header_t hdr, struct mu_hdrent *ent)
{
  char *s = MU_HDRENT_NAME (hdr, ent);
  s[ent->nlen] = ':';
  s = MU_HDRENT_VALUE (hdr, ent);
  s[ent->vlen] = '\n';
}

static void
mu_hdrent_unroll_fixup (mu_header_t hdr, struct mu_hdrent *ent)
{
  char *s = MU_HDRENT_NAME (hdr, ent);
  s[ent->nlen] = 0;
  s = MU_HDRENT_VALUE (hdr, ent);
  s[ent->vlen] = 0;
}

int
header_seek (mu_stream_t str, mu_off_t off, mu_off_t *presult)
{ 
  struct _mu_header_stream *hstr = (struct _mu_header_stream *) str;
  size_t size;
  int status;
    
  status = mu_header_size (hstr->hdr, &size);
  if (status)
    return status;
  
  if (off < 0 || off > size)
    return ESPIPE;
  hstr->off = off;
  *presult = off;
  return 0;
}

static int
header_read (mu_stream_t is, char *buffer, size_t buflen, size_t *pnread)
{
  struct _mu_header_stream *hstr = (struct _mu_header_stream *) is;
  mu_header_t header;
  struct mu_hdrent *ent;
  size_t ent_off;
  int status;
  size_t nread;
  
  if (is == NULL)
    return EINVAL;

  header = hstr->hdr;
  status = mu_header_fill (header);
  if (status)
    return status;

  if (header->spool_size == 0)
    {
      size_t len = 0;

      if (hstr->off == 0 && buflen > 0)
	{
	  /* Provide the trailing \n */
	  *buffer = '\n';
	  hstr->off++;
	  len = 1;
	}
      if (pnread)
	*pnread = len;
      return 0;
    }
  
  if (mu_hdrent_find_stream_pos (header, hstr->off, &ent, &ent_off))
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }

  for (nread = 0; nread < buflen && ent; ent = ent->next)
    {
      size_t rest = buflen - nread;
      size_t strsize = MU_STR_SIZE (ent->nlen, ent->vlen) - ent_off;
      if (rest > strsize)
	rest = strsize;
      mu_hdrent_fixup (header, ent);
      memcpy (buffer + nread, MU_HDRENT_NAME (header, ent) + ent_off, rest);
      mu_hdrent_unroll_fixup (header, ent);
      nread += rest;
      hstr->off += rest;
      ent_off = 0;
    }
  if (pnread)
    *pnread = nread;
  return 0;
}

#if 0
/* FIXME: Implement header_readdelim based on this: */
static int
_header_readline (mu_stream_t is, char *buffer, size_t buflen, size_t *pnread)
{
  struct _mu_header_stream *hstr = (struct _mu_header_stream *) is;
  mu_header_t header = hstr->hdr;
  struct mu_hdrent *ent;
  size_t ent_off;
  int status;
  size_t strsize;
  char *start, *end;

  if (buflen == 0)
    return EINVAL;

  header = mu_stream_get_owner (is);
  status = mu_header_fill (header);
  if (status)
    return status;
  if (mu_hdrent_find_stream_pos (header, hstr->off, &ent, &ent_off))
    {
      if (pnread)
	*pnread = 0;
      return 0;
    }

  buflen--; /* Account for the terminating nul */

  mu_hdrent_fixup (header, ent);
  strsize = MU_STR_SIZE (ent->nlen, ent->vlen) - ent_off;
  start = MU_HDRENT_NAME (header, ent) + ent_off;
  end = strchr (start, '\n');
  if (end)
    {
      size_t len = end - start + 1;
      if (len < strsize)
	strsize = len;
    }

  if (strsize < buflen)
    buflen = strsize;

  memcpy (buffer, start, buflen);
  buffer[buflen] = 0;
  hstr->off += buflen;
  mu_hdrent_unroll_fixup (header, ent);
  if (pnread)
    *pnread = buflen;
  return 0;
}
#endif

static int
header_write (mu_stream_t os, const char *buf, size_t buflen, size_t *pnwrite)
{
  struct _mu_header_stream *hstr = (struct _mu_header_stream *) os;
  mu_header_t header;
  int rc;
  
  if (os == NULL)
    return EINVAL;
  header = hstr->hdr;
  if (!(header->flags & HEADER_STREAMMOD))
    {
      struct mu_hdrent *ent;
	
      rc = mu_header_fill (header);
      if (rc)
	return rc;

      if (!header->mstream)
	{
	  rc = mu_memory_stream_create (&header->mstream, MU_STREAM_RDWR);
	  if (rc)
	    return rc;
	}
      mu_stream_seek (header->mstream, 0, MU_SEEK_SET, NULL);
      if (header->spool_used)
	{
	  for (ent = header->head; ent; ent = ent->next)
	    mu_hdrent_fixup (header, ent);
	  rc = mu_stream_write (header->mstream, header->spool,
				header->spool_used, NULL);
	  for (ent = header->head; ent; ent = ent->next)
	    mu_hdrent_unroll_fixup (header, ent);
	  if (rc)
	    return rc;
	  mu_stream_truncate (header->mstream, header->spool_used);
	  if (hstr->off > header->spool_used)
	    hstr->off = header->spool_used;
	}
      header->flags |= HEADER_STREAMMOD;
    }

  rc = mu_stream_seek (header->mstream, hstr->off, MU_SEEK_SET, NULL);
  if (rc)
    return rc;
  
  return mu_stream_write (header->mstream, buf, buflen, pnwrite);
}

static int
header_size (mu_stream_t str, mu_off_t *psize)
{
  mu_header_t header;
  int status;
  size_t size;
  
  if (str == NULL)
    return EINVAL;
  if (psize == NULL)
    return MU_ERR_OUT_PTR_NULL;
  
  header = ((struct _mu_header_stream *) str)->hdr;
  status = mu_header_fill (header);
  if (status)
    return status;
  status = mu_header_size (header, &size);
  if (status == 0)
    *psize = size;
  return status;
}

static int
_header_get_stream (mu_header_t header, mu_stream_t *pstream, int ref)
{
  if (header == NULL)
    return EINVAL;

  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;

  if (header->stream == NULL)
    {
      struct _mu_header_stream *str = 
	(struct _mu_header_stream *) _mu_stream_create (sizeof (*str),
							MU_STREAM_RDWR|MU_STREAM_SEEK);
      if (!str)
	return ENOMEM;
      str->stream.read = header_read;
      /*str->stream.rdelim? */
      str->stream.write = header_write;
      str->stream.seek = header_seek;
      str->stream.size = header_size;
      str->hdr = header;
      header->stream = (mu_stream_t) str;
    }
  if (!ref)
    {
      *pstream = header->stream;
      return 0;
    }
  return mu_streamref_create (pstream, header->stream);
}

int
mu_header_get_stream (mu_header_t header, mu_stream_t *pstream)
{
  /* FIXME: Deprecation warning */
  return _header_get_stream (header, pstream, 0);
}

int
mu_header_get_streamref (mu_header_t header, mu_stream_t *pstream)
{
  return _header_get_stream (header, pstream, 1);
}


int
mu_header_set_fill (mu_header_t header, int
		    (*_fill) (void *data, char **, size_t *),
		    void *data)
{
  if (header == NULL)
    return EINVAL;
  header->_fill = _fill;
  header->data = data;
  return 0;
}


int
mu_header_is_modified (mu_header_t header)
{
  return header ? (header->flags & HEADER_MODIFIED) : 0;
}

int
mu_header_clear_modified (mu_header_t header)
{
  if (header)
    header->flags &= ~HEADER_MODIFIED;
  return 0;
}


