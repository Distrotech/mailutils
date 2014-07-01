/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2014 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>

/* The "htmlent" filter converts the following special HTML entities:

    Decode       Encode
   ---------+------------- 
      <     +     &lt;
      >     +     &gt;
      &     +     &amp;
 */

struct transcode_map
{
  const char *ent;
  size_t len;
  int ch;
};

/* Note: entries in this array must be lexicographically sorted by the
   ent member. */
static struct transcode_map transcode_map[] = {
#define S(s) s, sizeof(s) - 1
  { S("&amp;"), '&' },
  { S("&gt;"), '>' },
  { S("&lt;"), '<' },
#undef S
  { NULL }
};

static struct transcode_map *
ch2ent (int c)
{
  struct transcode_map *p;

  for (p = transcode_map; p->ent; p++) 
    {
      if (p->ch == c)
	return p;
    }
  return NULL;
}

struct htmlent_encode_state
{
  char buf[6];
  int idx;
};

static enum mu_filter_result
_htmlent_encoder (void *xd,
		  enum mu_filter_command cmd,
		  struct mu_filter_io *iobuf)
{
  struct htmlent_encode_state *cp = xd;
  const char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
      cp->idx = -1;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }

  iptr = iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  while (isize && osize)
    {
      if (cp->idx > 0)
	{
	  *optr++ = cp->buf[--cp->idx];
	  --osize;
	}
      else
	{
	  struct transcode_map *p;
	  int c = *iptr++;
	  --isize;
	  
	  p = ch2ent (c);
	  if (p)
	    {
	      char const *q = p->ent + p->len;
	      cp->idx = 0;
	      while (q > p->ent)
		cp->buf[cp->idx++] = *--q;
	    }
	  else
	    {
	      *optr++ = c;
	      --osize;
	    }
	}
    }
  iobuf->isize -= isize;
  iobuf->osize -= osize;

  return mu_filter_ok;
}

enum htmlent_decode_phase
  {
    enc_init,
    enc_map,
    enc_rollback,
    enc_finish
  };

struct htmlent_decode_state
{
  enum htmlent_decode_phase phase;
  struct transcode_map *map;
  int idx;
  int pos;
};

#define DECODE_INIT(s)				\
  do						\
    {						\
      (s)->phase = enc_init;			\
      (s)->map = transcode_map;			\
      (s)->idx = 0;				\
    }						\
  while (0)

static enum htmlent_decode_phase
nextchar (struct htmlent_decode_state *s, int c)
{
  if (c == s->map->ent[s->idx])
    {
      if (++s->idx == s->map->len)
	s->phase = enc_finish;
      else
	s->phase = enc_map;
    }
  else
    {
      struct transcode_map *map;
      
      for (map = s->map; map->ent && c > map->ent[s->idx]; map++)
	;
      if (map->ent == NULL || c != map->ent[s->idx])
	{
	  if (s->idx != 0)
	    { 
	      s->phase = enc_rollback;
	      s->pos = 0;
	    }
	  else
	    DECODE_INIT (s);
	}
      else
	{
	  s->map = map;
	  if (++s->idx == s->map->len)
	    s->phase = enc_finish;
	  else
	    s->phase = enc_map;
	}
    }
  return s->phase;
}

static enum mu_filter_result
_htmlent_decoder (void *xd,
		  enum mu_filter_command cmd,
		  struct mu_filter_io *iobuf)
{
  struct htmlent_decode_state *cp = xd;
  const char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
      DECODE_INIT (cp);
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }

  iptr = iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  while (isize && osize)
    {
      switch (cp->phase)
	{
	case enc_init:
	  nextchar (cp, *iptr);
	  if (cp->phase == enc_init)
	    {
	      *optr++ = *iptr;
	      --osize;
	    }
	  ++iptr;
	  --isize;
	  break;
	      
	case enc_map:
	  nextchar (cp, *iptr);
	  if (cp->phase == enc_map || cp->phase == enc_finish)
	    {
	      ++iptr;
	      --isize;
	    }
	  else if (cp->phase == enc_init)
	    {
	      *optr++ = *iptr++;
	      --osize;
	      --isize;
	    }
	  break;

	case enc_finish:
	  *optr++ = cp->map->ch;
	  --osize;
	  DECODE_INIT (cp);
	  break;
	  
	case enc_rollback:
	  *optr++ = cp->map->ent[cp->pos];
	  --osize;
	  if (++cp->pos == cp->idx)
	    DECODE_INIT (cp);
	  break;
	}	  
    }      
  iobuf->isize -= isize;
  iobuf->osize -= osize;

  return mu_filter_ok;  
}

static int
alloc_state (void **pret, int mode, int argc MU_ARG_UNUSED,
	     const char **argv MU_ARG_UNUSED)
{
  union
  {
    struct htmlent_decode_state decode;
    struct htmlent_encode_state encode;
  } *cp;
    

  switch (mode)
    {
    case MU_FILTER_ENCODE:
      cp = malloc (sizeof (cp->encode));
      if (!cp)
	return ENOMEM;
      cp->encode.idx = -1;
      break;

    case MU_FILTER_DECODE:
      cp = malloc (sizeof (cp->decode));
      if (!cp)
	return ENOMEM;
      cp->decode.idx = 0;
      cp->decode.map = transcode_map;
      break;

    default:
      abort ();
    }
      
  *pret = cp;
  return 0;
}

static struct _mu_filter_record _htmlent_filter = {
  "htmlent",
  alloc_state,
  _htmlent_encoder,
  _htmlent_decoder,
};

mu_filter_record_t mu_htmlent_filter = &_htmlent_filter;

