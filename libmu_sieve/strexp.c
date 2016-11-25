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

/* String expander */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif  
#include <sieve-priv.h>

enum segm_type
  {
    segm_copy,       /* reference to a fragment of the source string */
    segm_repl        /* replacement */
  };

struct string_segment
{
  enum segm_type type;  /* Segment type */
  size_t beg;           /* Beginning of the source string fragmen */
  size_t end;           /* End of it */
  char *repl;           /* Replacement, if type == segm_repl */
};

struct stringbuf
{
  char const *string;   /* Source string */
  size_t length;        /* Length of the source */
  size_t pos;           /* Offset of the current character */
  
  mu_list_t seglist;    /* List of segments */
  jmp_buf errbuf;       /* Return location on failure */

  char *expansion;      /* Expanded string */
  char *endptr;         /* Used when assembling expansion */
  
  mu_i_sv_interp_t interp;
  void *data;
};

static struct string_segment *
segment_alloc (struct stringbuf *buf, size_t beg, enum segm_type type)
{
  int rc;
  struct string_segment *segm;

  segm = malloc (sizeof *segm);
  if (!segm)
    longjmp (buf->errbuf, ENOMEM);
  segm->type = type;
  segm->beg = beg;
  segm->end = buf->pos - 1;
  rc = mu_list_append (buf->seglist, segm);
  if (rc)
    longjmp (buf->errbuf, rc);
  return segm;
}

static void
segm_free (void *data)
{
  struct string_segment *segm = data;
  if (segm->type == segm_repl)
    free (segm->repl);
  free (segm);
}

static void
string_next_fragment (struct stringbuf *buf)
{
  size_t beg;
  struct string_segment *segm;
  char *exp;
  
  beg = buf->pos;
  while (buf->pos < buf->length)
    {
      if (buf->string[buf->pos] == '$' && buf->pos + 1 < buf->length
	  && buf->string[buf->pos + 1] == '{')
	break;
      buf->pos++;
    }
  segm = segment_alloc (buf, beg, segm_copy);
  if (buf->pos == buf->length)
    return;

  beg = buf->pos;
  buf->pos += 2;
  /* Look for closing brace */
  while (buf->pos < buf->length)
    {
      if (buf->string[buf->pos] == '$' && buf->pos + 1 < buf->length
	  && buf->string[buf->pos + 1] == '{')
	{
	  /* Found nested reference.  Update verbatim segment and restart */
	  segm->end = buf->pos - 1;
	  beg = buf->pos;
	  buf->pos++;
	}
      else if (buf->string[buf->pos] == '}')
	break;
      buf->pos++;
    }

  if (buf->pos == buf->length)
    {
      /* No references found */
      segm->end = buf->pos - 1;
      return;
    }

  if (buf->interp (buf->string + beg + 2, buf->pos - beg - 2, &exp,
		   buf->data) == 0)
    {
      segm = segment_alloc (buf, beg, segm_repl);
      segm->repl = exp;
    }
  else
    segm->end = buf->pos;

  buf->pos++;
}

struct segm_stat
{
  size_t end;
  size_t len;
};

static int
update_len (void *item, void *data)
{
  struct string_segment *segm = item;
  struct segm_stat *st = data;

  switch (segm->type)
    {
    case segm_copy:
      if (segm->beg == st->end)
	st->end = segm->end;
      st->len += segm->end - segm->beg + 1;
      break;

    case segm_repl:
      st->len += strlen (segm->repl);
      break;
    }
  return 0;
}

static int
append_segm (void *item, void *data)
{
  struct string_segment *segm = item;
  struct stringbuf *buf = data;
  size_t len;
  
  switch (segm->type)
    {
    case segm_copy:
      len = segm->end - segm->beg + 1;
      memcpy (buf->endptr, buf->string + segm->beg, len);
      break;

    case segm_repl:
      len = strlen (segm->repl);
      memcpy (buf->endptr, segm->repl, len);
    }
  
  buf->endptr += len;
  return 0;
}

static void
string_split (struct stringbuf *buf)
{
  while (buf->pos < buf->length)
    string_next_fragment (buf);
}

static int
string_assemble (struct stringbuf *buf)
{
  struct segm_stat st;  
  st.len = 0;
  st.end = 0;
  
  mu_list_foreach (buf->seglist, update_len, &st);
  if (st.end == buf->length - 1)
    return MU_ERR_CANCELED;

  buf->expansion = malloc (st.len + 1);
  if (!buf->expansion)
    longjmp (buf->errbuf, ENOMEM);
  buf->endptr = buf->expansion; 
  mu_list_foreach (buf->seglist, append_segm, buf);
  *buf->endptr = 0;

  return 0;
}

int
mu_i_sv_string_expand (char const *input,
		       mu_i_sv_interp_t interp, void *data, char **ret)
{
  struct stringbuf sb;
  int rc;
  
  sb.string = input;
  sb.length = strlen (input);
  sb.pos = 0;

  rc = mu_list_create (&sb.seglist);
  if (rc)
    return rc;
  mu_list_set_destroy_item (sb.seglist, segm_free);
  
  sb.expansion = NULL;
  sb.endptr = NULL;

  sb.interp = interp;
  sb.data = data;
  
  rc = setjmp (sb.errbuf);
  if (rc == 0)
    {
      string_split (&sb);
      rc = string_assemble (&sb);
      if (rc == 0)
	*ret = sb.expansion;
    }
  mu_list_destroy (&sb.seglist);
  return rc;
}

  
  
