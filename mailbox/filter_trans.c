/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

/* Notes:
First Draft: Dave Inglis.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/stream.h>

#include <filter0.h>

#define MU_TRANS_DECODE		1
#define MU_TRANS_ENCODE		2
#define MU_TRANS_BSIZE		2048

static int qp_init __P ((filter_t));
static int base64_init __P ((filter_t));

static struct _filter_record _qp_filter =
{
  "quoted-printable",
  qp_init,
  NULL,
  NULL,
  NULL
};

static struct _filter_record _base64_filter =
{
  "base64",
  base64_init,
  NULL,
  NULL,
  NULL
};

static struct _filter_record _bit8_filter =
{
  "8bit",
  NULL,
  NULL,
  NULL,
  NULL
};

static struct _filter_record _bit7_filter =
{
  "7bit",
  NULL,
  NULL,
  NULL,
  NULL
};

static struct _filter_record _binary_filter =
{
  "binary",
  NULL,
  NULL,
  NULL,
  NULL
};

/* Export.  */
filter_record_t qp_filter = &_qp_filter;
filter_record_t base64_filter = &_base64_filter;
filter_record_t binary_filter = &_binary_filter;
filter_record_t bit8_filter = &_bit8_filter;
filter_record_t bit7_filter = &_bit7_filter;

struct _trans_stream
{
  int t_offset; /* Orignal stream offset.  */

  size_t min_size;
  int s_offset;
  char *s_buf;     /* Used when read if not big enough to handle min_size
		      for decoder/encoder */

  int offset;      /* Current stream offset */
  int line_len;

  int w_rhd;       /* Working buffer read ahead  */
  int w_whd;       /* Working buffer write ahead */
  char w_buf[MU_TRANS_BSIZE]; /* working source/dest buffer */

  int (*transcoder) __P ((const char *iptr, size_t isize, char *optr,
			  size_t osize, size_t *nbytes, int *line_len));
};

static int base64_decode __P ((const char *iptr, size_t isize, char *optr,
				size_t osize, size_t *nbytes, int *line_len));
static int base64_encode __P ((const char *iptr, size_t isize, char *optr,
				size_t osize, size_t *nbytes, int *line_len));

static int qp_decode __P ((const char *iptr, size_t isize, char *optr,
			    size_t osize, size_t *nbytes, int *line_len));
static int qp_encode __P ((const char *iptr, size_t isize, char *optr,
			    size_t osize, size_t *nbytes, int *line_len));

static void
trans_destroy (filter_t filter)
{
  struct _trans_stream *ts = filter->data;
  if (ts->s_buf)
    free (ts->s_buf);
  free (ts);
}

static int
trans_read (filter_t filter, char *optr, size_t osize, off_t offset,
	     size_t *n_bytes)
{
  struct _trans_stream *ts = filter->data;
  size_t obytes, wbytes = 0;
  int ret = 0, i;
  size_t bytes, *nbytes = &bytes;

  if (optr == NULL || osize == 0)
    return EINVAL;

  if (n_bytes)
    nbytes = n_bytes;
  *nbytes = 0;

  if (offset && ts->t_offset != offset)
    return ESPIPE;

  if (offset == 0)
    ts->s_offset = ts->t_offset = ts->w_whd = ts->w_rhd =
      ts->offset = ts->line_len = 0;

  while (*nbytes < osize)
    {
      if ((ts->w_rhd + (int)ts->min_size) >= ts->w_whd)
	{
	  memmove (ts->w_buf, ts->w_buf + ts->w_rhd, ts->w_whd - ts->w_rhd);
	  ts->w_whd = ts->w_whd - ts->w_rhd;
	  ts->w_rhd = 0;
	  ret = stream_read (filter->stream, ts->w_buf + ts->w_whd,
			     MU_TRANS_BSIZE - ts->w_whd, ts->offset,
			     &wbytes );
	  if (ret != 0)
	    break;
	  ts->offset += wbytes;
	  ts->w_whd += wbytes;
	}
      if ((osize - *nbytes) >= ts->min_size
	  && ts->s_offset == 0
	  && ts->w_whd - ts->w_rhd)
	{
	  ts->w_rhd += ts->transcoder (ts->w_buf + ts->w_rhd,
				       ts->w_whd - ts->w_rhd,
				       optr + *nbytes, osize - *nbytes,
				       &obytes, &ts->line_len);
	  if (ts->w_rhd > ts->w_whd) /* over shot due to padding */
	    ts->w_rhd = ts->w_whd;
	  *nbytes += obytes;
	  ts->t_offset += obytes;
	}
      else
	{
	  if (ts->s_offset == 0 && ts->w_whd - ts->w_rhd)
	    {
	      ts->w_rhd += ts->transcoder (ts->w_buf + ts->w_rhd,
					   ts->w_whd - ts->w_rhd, ts->s_buf,
					   ts->min_size, &obytes,
					   &ts->line_len);
	      if (ts->w_rhd > ts->w_whd) /* over shot due to padding */
		ts->w_rhd = ts->w_whd;
	      ts->s_offset = obytes;
	    }
	  for (i = ts->s_offset; i > 0; i--)
	    {
	      optr[(*nbytes)++] = ts->s_buf[ts->s_offset - i];
	      ts->t_offset++;
	      if (*nbytes >= osize)
		{
		  i--;
		  memmove (ts->s_buf, &ts->s_buf[ts->s_offset - i], i);
		  break;
		}
	    }
	  ts->s_offset = i;
	}
      if (wbytes == 0 && ts->s_offset == 0)
	break;
    }
  return ret;
}

#if 0
static int
trans_write (stream_t stream, const char *iptr, size_t isize, off_t offset,
	      size_t *nbytes)
{
  struct _trans_stream *ts = stream_get_owner (stream);
  return stream_write (ts->stream, iptr, isize, offset, nbytes);
}
#endif


/*------------------------------------------------------
 * base64 encode/decode
 *----------------------------------------------------*/

static int
b64_input (char c)
{
  const char table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i;

  for (i = 0; i < 64; i++)
    {
      if (table[i] == c)
	return i;
    }
  return -1;
}

static int
base64_init (filter_t filter)
{
  struct _trans_stream *ts;
  ts = calloc (sizeof (*ts), 1);
  if (ts == NULL)
    return ENOMEM;

  ts->min_size = 4;
  ts->s_buf = calloc (ts->min_size, 1);
  if (ts->s_buf == NULL)
    {
      free (ts);
      return ENOMEM;
    }
  ts->transcoder = (filter->type == MU_FILTER_DECODE) ? base64_decode : base64_encode;

  filter->_read = trans_read;
  filter->_destroy = trans_destroy;
  filter->data = ts;
  return 0;
}

static int
base64_decode (const char *iptr, size_t isize, char *optr, size_t osize,
	       size_t *nbytes, int *line_len ARG_UNUSED)
{
  int i = 0, tmp = 0, pad = 0;
  size_t consumed = 0;
  unsigned char data[4];

  *nbytes = 0;
  while (consumed < isize && (*nbytes)+3 < osize)
    {
      while (( i < 4 ) && (consumed < isize))
	{
	  tmp = b64_input (*iptr++);
	  consumed++;
	  if (tmp != -1)
	    data[i++] = tmp;
	  else if (*(iptr-1) == '=')
	    {
	      data[i++] = '\0';
	      pad++;
	    }
	}

      /* I have a entire block of data 32 bits get the output data.  */
      if (i == 4)
	{
	  *optr++ = (data[0] << 2) | ((data[1] & 0x30) >> 4);
	  *optr++ = ((data[1] & 0xf) << 4) | ((data[2] & 0x3c) >> 2);
	  *optr++ = ((data[2] & 0x3) << 6) | data[3];
	  (*nbytes) += 3 - pad;
	}
      else
	{
	  /* I did not get all the data.  */
	  consumed -= i;
	  return consumed;
	}
      i = 0;
    }
  return consumed;
}

#define BASE64_LINE_MAX 	77
static int
base64_encode (const char *iptr, size_t isize, char *optr, size_t osize,
	       size_t *nbytes, int *line_len)
{
  size_t consumed = 0;
  int pad = 0;
  const char *b64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const unsigned char* ptr = (const unsigned char*) iptr;
	
  *nbytes = 0;
  if (isize <= 3)
    pad = 1;
  while (((consumed + 3) <= isize && (*nbytes + 4) <= osize) || pad)
    {
      if (*line_len == 76)
	{
	  *optr++ = '\n';
	  (*nbytes)++;
	  (*line_len) = 0;
	  if ((*nbytes + 4) > osize)
	    return consumed;
	}
      *optr++ = b64[ptr[0] >> 2];
      *optr++ = b64[((ptr[0] << 4) + (--isize ? (ptr[1] >> 4): 0)) & 0x3f];
      *optr++ = isize ? b64[((ptr[1] << 2) + (--isize ? (ptr[2] >> 6) : 0 )) & 0x3f] : '=';
      *optr++ = isize ? b64[ptr[2] & 0x3f] : '=';
      ptr += 3;
      consumed += 3;
      (*nbytes) += 4;
      (*line_len) +=4;
      pad = 0;
    }
  return consumed;
}

/*------------------------------------------------------
 * quoted-printable decoder/encoder
 *------------------------------------------------------*/
static const char _hexdigits[] = "0123456789ABCDEF";

#define QP_LINE_MAX	76
#define ISWS(c) ((c)==' ' || (c)=='\t')

static int
qp_init (filter_t filter)
{
  struct _trans_stream *ts;
  ts = calloc (sizeof (*ts), 1);
  if (ts == NULL)
    return ENOMEM;

  ts->min_size = QP_LINE_MAX;
  ts->s_buf = calloc (ts->min_size, 1);
  if (ts->s_buf == NULL)
    {
      free (ts);
      return ENOMEM;
    }
  ts->transcoder = (filter->type == MU_FILTER_DECODE) ? qp_decode : qp_encode;

  filter->_read = trans_read;
  filter->_destroy = trans_destroy;
  filter->data = ts;
  return 0;
}

static int
qp_decode (const char *iptr, size_t isize, char *optr, size_t osize,
	   size_t *nbytes, int *line_len ARG_UNUSED)
{
  char c;
  int last_char = 0;
  size_t consumed = 0;
  size_t wscount = 0;
  
  *nbytes = 0;
  while (consumed < isize && *nbytes < osize)
    {
      c = *iptr++;

      if (ISWS (c))
	{
	  wscount++;
	  consumed++;
	}
      else
	{
	  /* Octets with values of 9 and 32 MAY be
	     represented as US-ASCII TAB (HT) and SPACE characters,
	     respectively, but MUST NOT be so represented at the end
	     of an encoded line.  Any TAB (HT) or SPACE characters
	     on an encoded line MUST thus be followed on that line
	     by a printable character. */
	  
	  if (wscount)
	    {
	      if (c != '\r' && c != '\n')
		{
		  size_t sz;
		  
		  if (consumed >= isize)
		    break;

		  if (*nbytes + wscount > osize)
		    sz = osize - *nbytes;
		  else
		    sz = wscount;
		  memcpy (optr, iptr - wscount - 1, sz);
		  optr += sz;
		  (*nbytes) += sz;
		  if (wscount > sz)
		    {
		      wscount -= sz;
		      break;
		    }
		}
	      wscount = 0;
	    }
		
	  if (c == '=')
	    {
	      /* There must be 2 more characters before I consume this.  */
	      if (consumed + 2 >= isize)
		break;
	      else
		{
		  /* you get =XX where XX are hex characters.  */
		  char 	chr[3];
		  int 	new_c;

		  chr[2] = 0;
		  chr[0] = *iptr++;
		  /* Ignore LF.  */
		  if (chr[0] != '\n')
		    {
		      chr[1] = *iptr++;
		      new_c = strtoul (chr, NULL, 16);
		      *optr++ = new_c;
		      (*nbytes)++;
		      consumed += 3;
		    }
		  else
		    consumed += 2;
		}
	    }
	  /* CR character.  */
	  else if (c == '\r')
	    {
	      /* There must be at least 1 more character before
		 I consume this.  */
	      if (consumed + 1 >= isize)
		break;
	      else
		{
		  iptr++; /* Skip the CR character.  */
		  *optr++ = '\n';
		  (*nbytes)++;
		  consumed += 2;
		}
	    }
	  else
	    {
	      *optr++ = c;
	      (*nbytes)++;
	      consumed++;
	    }
	}	  
      last_char = c;
    }
  return consumed - wscount;
}

#define SOFTBRK() \
      /* check if we have enough room for the soft linebreak */\
      if (*nbytes + 2 > osize) \
  	break;\
      *optr++ = '=';\
      *optr++ = '\n';\
      (*nbytes) += 2;\
      *line_len = 0;

static int
qp_encode (const char *iptr, size_t isize, char *optr, size_t osize,
	   size_t *nbytes, int *line_len)
{
  unsigned int c;
  size_t consumed = 0;
  size_t wscount = 0;

  *nbytes = 0;

  /* Strategy: check if we have enough room in the output buffer only
     once the required size has been computed. If there is not enough,
     return and hope that the caller will free up the output buffer a
     bit. */

  while (consumed < isize)
    {
      int simple_char;
      
      /* candidate byte to convert */
      c = *(unsigned char*) iptr;
      simple_char = (c >= 32 && c <= 60)
     	             || (c >= 62 && c <= 126)
	             || c == '\t'
	             || c == '\n';

      if (*line_len == QP_LINE_MAX
	  || (c == '\n' && consumed && ISWS (optr[-1]))
	  || (!simple_char && *line_len >= (QP_LINE_MAX - 3)))
	{
	  /* to cut a qp line requires two bytes */
	  if (*nbytes + 2 > osize) 
	    break;

	  *optr++ = '=';
	  *optr++ = '\n';
	  (*nbytes) += 2;
	  *line_len = 0;
	}
	  
      if (ISWS (c))
	wscount++;
	
      if (simple_char)
	{
	  /* a non-quoted character uses up one byte */
	  if (*nbytes + 1 > osize) 
	    break;

	  *optr++ = c;
	  (*nbytes)++;
	  (*line_len)++;

	  iptr++;
	  consumed++;

	  if (!ISWS (c))
	    wscount = 0;
	  
	  if (c == '\n')
	    *line_len = 0;
	}
      else
	{
	  /* a quoted character uses up three bytes */
	  if ((*nbytes) + 3 > osize) 
	    break;

	  *optr++ = '=';
	  *optr++ = _hexdigits[(c >> 4) & 0xf];
	  *optr++ = _hexdigits[c & 0xf];

	  (*nbytes) += 3;
	  (*line_len) += 3;

	  /* we've actuall used up one byte of input */
	  iptr++;
	  consumed++;
	  wscount = 0;
	}
    }
  if (wscount)
    wscount--;
  (*nbytes) -= wscount;
  return consumed - wscount;
}
