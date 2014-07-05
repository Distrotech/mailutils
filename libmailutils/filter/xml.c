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
#include <stdio.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/cctype.h>

/* In the "encode" mode, the "xml" filter takes a stream of UTF-8
   characters as its input, converts the three XML entities as per
   table below and replaces invalid characters (see
   http://www.w3.org/TR/xml/#dt-charref) with their numeric character
   reference representation as per http://www.w3.org/TR/xml/#charencoding.

   In decode mode, the filter does the reverse.
   
   Special conversions:
   
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

struct xml_encode_state
{
  char buf[11]; /* Enough to acomodate &#x10FFFF; - max. UTF-8 codepoint */
  int idx;
};

static int
utf8_char_width(unsigned char ch)
{
    if (ch <= 0x7f)
        return 1;
    if (0xc2 <= ch && ch <= 0xdf)
        return 2;
    if (0xe0 <= ch && ch <= 0xef)
        return 3;
    if (0xf0 <= ch && ch <= 0xf4)
        return 4;
    return 0;
}

#ifndef ENODATA
# define ENODATA EINVAL
#endif

static int
utf8_mbtowc (unsigned char const *s, size_t l, unsigned int *pwc)
{
  if (l == 0)
    {
      errno = ENODATA;
      return -1;
    }
  
  if (s[0] < 0x80)
    {
      *pwc = s[0];
      return 1;
    }

  if (s[0] < 0xc2)
    {
      errno = EILSEQ;
      return -1;
    }
  else if (s[0] < 0xe0)
    {
      if (l < 2)
        {
	  errno = ENODATA;
	  return -1;
	}
      if (!((s[1] ^ 0x80) < 0x40))
	{
	  errno = EILSEQ;
	  return -1;
        }
      *pwc = ((unsigned int) (s[0] & 0x1f) << 6) |
	      (unsigned int) (s[1] ^ 0x80);
      return 2;
    }

  if (s[0] < 0xf0)
    {
      if (l < 3)
        {
	  errno = ENODATA;
	  return -1;
	}

      if (!((s[1] ^ 0x80) < 0x40
	    && (s[2] ^ 0x80) < 0x40 && (s[0] >= 0xe1 || s[1] >= 0xa0)))
	{
	  errno = EILSEQ;
	  return -1;
        }
      
      *pwc = ((unsigned long) (s[0] & 0x0f) << 12) |
              ((unsigned long) (s[1] ^ 0x80) << 6) |
               (unsigned long) (s[2] ^ 0x80);

      return 3;
    }
    if (s[0] < 0xf8)
      {
	if (l < 4)
	  {
	    errno = ENODATA;
	    return -1;
	  }

        if (!((s[1] ^ 0x80) < 0x40 &&
              (s[2] ^ 0x80) < 0x40 &&
              (s[3] ^ 0x80) < 0x40 && (s[0] >= 0xf1 || s[1] >= 0x90)))
	  {
            errno = EILSEQ;
            return -1;
	  }

        *pwc = ((unsigned long) (s[0] & 0x07) << 18) |
                ((unsigned long) (s[1] ^ 0x80) << 12) |
                ((unsigned long) (s[2] ^ 0x80) << 6) |
                (unsigned long) (s[3] ^ 0x80);

        return 4;
      }
    errno = EILSEQ;
    return -1;
}

static void
store_buf (struct xml_encode_state *cp, const char *s, size_t len)
{
  cp->idx = 0;
  s += len;
  while (len--)
    cp->buf[cp->idx++] = *--s;
}

static void
store_char_ref (struct xml_encode_state *cp, unsigned wc)
{
  char buf[11];
  snprintf (buf, sizeof buf, "&#x%x;", wc);
  store_buf (cp, buf, strlen (buf));
}

static enum mu_filter_result
_xml_encoder (void *xd,
	      enum mu_filter_command cmd,
	      struct mu_filter_io *iobuf)
{
  struct xml_encode_state *cp = xd;
  const unsigned char *iptr;
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

  iptr = (unsigned char*) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  while (osize)
    {
      if (cp->idx > 0)
	{
	  *optr++ = cp->buf[--cp->idx];
	  --osize;
	}
      else if (isize == 0)
	break;
      else
	{
	  struct transcode_map *p;
	  unsigned int c = *iptr;

	  p = ch2ent (c);
	  if (p)
	    {
	      store_buf (cp, p->ent, p->len);
	      ++iptr;
	      --isize;
	    }
	  else
	    {
	      int count = utf8_char_width (c);

	      if (count == 0)
		{
		  store_char_ref (cp, *iptr);
		  ++iptr;
		  --isize;
		}
	      else if (count > isize)
		{
		  if (cmd == mu_filter_lastbuf)
		    {
		      store_buf (cp, (char*) iptr, isize);
		      iptr += isize;
		      isize = 0;
		    }
		  else
		    break;
		}
	      else
		{
		  unsigned wc;
		  int rc;

		  rc = utf8_mbtowc (iptr, isize, &wc);
		  
		  if (rc == -1)
		    {	
		      store_char_ref (cp, *iptr);
		      ++iptr;
		      --isize;
		    }
		  else
		    {
/* http://www.w3.org/TR/xml/#dt-charref:

   Character Range

   Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] |
            [#xE000-#xFFFD] | [#x10000-#x10FFFF]

   (any Unicode character, excluding the surrogate blocks, FFFE, and FFFF)
*/
		      if (wc == 0x9
			  || wc == 0xa
			  || wc == 0xd
			  || (wc >= 0x20 && wc < 0xd7ff)
			  || (wc >= 0xe000 && wc < 0xfffd)
			  || (wc >= 0x10000 && wc < 0x10FFFF))
			{
			  if (osize >= count)
			    {
			      memcpy (optr, iptr, count);
			      
			      optr += count;
			      osize -= count;

			      iptr += count;
			      isize -= count;
			    }
			  else
			    {
			      store_buf (cp, (char*) iptr, count);
			      iptr += count;
			      isize -= count;
			    }
			}
		      else
			{
                          store_char_ref (cp, wc);
			  iptr += count;
			  isize -= count;
			}
		    }
		}
	    }
	}
    }
  iobuf->isize -= isize;
  iobuf->osize -= osize;

  return mu_filter_ok;
}

enum xml_decode_phase
  {
    enc_init,
    enc_map,
    enc_rollback,
    enc_finish,
    enc_char_ref,
    enc_char_cont,
    enc_char_rollback
  };

struct xml_decode_state
{
  enum xml_decode_phase phase;   /* Decoding phase */
  struct transcode_map *map;     /* Map entry being tried */
  char buf[11]; /* Buffer for storing &# sequence -- used for rollback.
		   Regarding its dimension, see the comment to
		   xml_encode_state above. */
  int idx;      /* In enc_init and enc_map phase - index to the next
		   character in map->ent to compare;
		   In enc_rollback phase marks the point in map->ent where
		   to stop rolling back.
		   In enc_char_ref and enc_char_cont modes indicates next
		   free slot in buf.
		   In enc_char_rollback phase marks the point in buf where
		   to stop rolling back.
		*/
  int pos;      /* In enc_rollback phase - position in
		   map->ent of the next character  to restore.
		   In enc_char_rollback phase - position in buf of the next
		   character  to restore. 
		*/
  int base;     /* Base for converting &# references */
  unsigned wc;  /* Wide char being constructed from a reference */
};

#define DECODE_INIT(s)				\
  do						\
    {						\
      (s)->phase = enc_init;			\
      (s)->map = transcode_map;			\
      (s)->idx = 0;				\
    }						\
  while (0)

static enum xml_decode_phase
nextchar (struct xml_decode_state *s, int c)
{
  if (c == s->map->ent[s->idx])
    {
      if (++s->idx == s->map->len)
	s->phase = enc_finish;
      else
	s->phase = enc_map;
    }
  else if (s->phase == enc_init)
    /* nothing */;
  else if (s->idx == 1 && c == '#')
    {
      s->phase = enc_char_ref;
      s->base = 0;
      s->wc = 0;
      s->idx = 0;
      s->buf[s->idx++] = '&';
      s->buf[s->idx++] = c;
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
_xml_decoder (void *xd,
	      enum mu_filter_command cmd,
	      struct mu_filter_io *iobuf)
{
  struct xml_decode_state *cp = xd;
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
	  if (cp->phase == enc_map || cp->phase == enc_finish
	      || cp->phase == enc_char_ref)
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

	case enc_char_ref:
	  if (*iptr == 'x')
	    {
	      cp->base = 16;
	      cp->buf[cp->idx++] = *iptr;
	      ++iptr;
	      --isize;
	    }
	  else
	    cp->base = 10;
	  cp->phase = enc_char_cont;
	  break;

	case enc_char_cont:
	  if (*iptr == ';')
	    {
	      *optr++ = cp->wc;
	      --osize;
	      ++iptr;
	      --isize;
	      DECODE_INIT (cp);
	    }
	  else if (cp->idx < MU_ARRAY_SIZE (cp->buf) && mu_isxdigit (*iptr))
	    {
	      static char xdig[] = "0123456789ABCDEF";
	      int n = strchr (xdig, mu_toupper (*iptr)) - xdig;
	      cp->wc = cp->wc * cp->base + n;
	      cp->buf[cp->idx++] = *iptr;
	      ++iptr;
	      --isize;
	    }
	  else
	    {
	      cp->phase = enc_char_rollback;
	      cp->pos = 0;
	    }
	  break;

	case enc_char_rollback:
	  *optr++ = cp->buf[cp->pos++];
	  --osize;
	  if (cp->pos == cp->idx)
	    DECODE_INIT (cp);
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
    struct xml_encode_state encode;
    struct xml_decode_state decode;
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

static struct _mu_filter_record _xml_filter = {
  "xml",
  alloc_state,
  _xml_encoder,
  _xml_decoder
};

mu_filter_record_t mu_xml_filter = &_xml_filter;


