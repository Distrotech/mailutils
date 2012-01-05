/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2002, 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>

static char b64tab[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int b64val[128] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

int
mu_base64_encode (const unsigned char *input, size_t input_len,
		  unsigned char **output, size_t *output_len)
{
  size_t olen = 4 * (input_len + 2) / 3 + 1;
  unsigned char *out = malloc (olen);

  if (!out)
    return ENOMEM;
  *output = out;
  while (input_len >= 3)
    {
      *out++ = b64tab[input[0] >> 2];
      *out++ = b64tab[((input[0] << 4) & 0x30) | (input[1] >> 4)];
      *out++ = b64tab[((input[1] << 2) & 0x3c) | (input[2] >> 6)];
      *out++ = b64tab[input[2] & 0x3f];
      input_len -= 3;
      input += 3;
    }

  if (input_len > 0)
    {
      unsigned char c = (input[0] << 4) & 0x30;
      *out++ = b64tab[input[0] >> 2];
      if (input_len > 1)
	c |= input[1] >> 4;
      *out++ = b64tab[c];
      *out++ = (input_len < 2) ? '=' : b64tab[(input[1] << 2) & 0x3c];
      *out++ = '=';
    }
  *output_len = out - *output;
  *out = 0;
  return 0;
}

int
mu_base64_decode (const unsigned char *input, size_t input_len,
		  unsigned char **output, size_t *output_len)
{
  int olen = input_len;
  unsigned char *out = malloc (olen);

  if (!out)
    return ENOMEM;
  *output = out;
  do
    {
      if (input[0] > 127 || b64val[input[0]] == -1
	  || input[1] > 127 || b64val[input[1]] == -1
	  || input[2] > 127 || ((input[2] != '=') && (b64val[input[2]] == -1))
	  || input[3] > 127 || ((input[3] != '=')
				&& (b64val[input[3]] == -1)))
	return EINVAL;
      *out++ = (b64val[input[0]] << 2) | (b64val[input[1]] >> 4);
      if (input[2] != '=')
	{
	  *out++ = ((b64val[input[1]] << 4) & 0xf0) | (b64val[input[2]] >> 2);
	  if (input[3] != '=')
	    *out++ = ((b64val[input[2]] << 6) & 0xc0) | b64val[input[3]];
	}
      input += 4;
      input_len -= 4;
    }
  while (input_len > 0);
  *output_len = out - *output;
  return 0;
}


static enum mu_filter_result
_base64_decoder (void *xd MU_ARG_UNUSED,
		 enum mu_filter_command cmd,
		 struct mu_filter_io *iobuf)
{
  enum mu_filter_result result = mu_filter_ok;
  int i = 0, tmp = 0, pad = 0;
  size_t consumed = 0;
  unsigned char data[4];
  size_t nbytes = 0;
  const char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }
  
  if (iobuf->osize <= 3)
    {
      iobuf->osize = 3;
      return mu_filter_moreoutput;
    }

  iptr = iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;
 
  while (consumed < isize && nbytes + 3 < osize)
    {
      while (i < 4 && consumed < isize)
	{
	  tmp = b64val[*(const unsigned char*)iptr++];
	  consumed++;
	  if (tmp != -1)
	    data[i++] = tmp;
	  else if (*(iptr-1) == '=')
	    {
	      data[i++] = 0;
	      pad++;
	    }
	}

      /* I have a entire block of data 32 bits get the output data.  */
      if (i == 4)
	{
	  *optr++ = (data[0] << 2) | ((data[1] & 0x30) >> 4);
	  *optr++ = ((data[1] & 0xf) << 4) | ((data[2] & 0x3c) >> 2);
	  *optr++ = ((data[2] & 0x3) << 6) | data[3];
	  nbytes += 3 - pad;
	}
      else
	{
	  /* I did not get all the data.  */
	  if (cmd != mu_filter_lastbuf)
	    {
	      consumed -= i;
	      result = mu_filter_again;
	    }
	  break;
	}
      i = 0;
    }
  iobuf->isize = consumed;
  iobuf->osize = nbytes;
  return result;
}

enum base64_state
{
  base64_init,
  base64_newline,
  base64_rollback
};

struct base64_line
{
  enum base64_state state;
  size_t max_len;
  size_t cur_len;
  char save[3];
  int idx;
};

static enum mu_filter_result
_base64_encoder (void *xd,
		 enum mu_filter_command cmd,
		 struct mu_filter_io *iobuf)
{
  struct base64_line bline, *lp = xd;
  size_t consumed = 0;
  int pad = 0;
  const unsigned char *ptr;
  size_t nbytes = 0;
  size_t isize;
  char *optr;
  size_t osize;
  enum mu_filter_result res;

  if (!lp)
    {
      lp = &bline;
      lp->max_len = 0;
      lp->state = base64_init;
    }
  switch (cmd)
    {
    case mu_filter_init:
      lp->state = base64_init;
      lp->cur_len = 0;
      lp->idx = 3;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }
  
  if (iobuf->isize <= 3)
    {
      if (cmd == mu_filter_lastbuf)
	pad = 1;
      else
	{
	  iobuf->isize = 4;
	  return mu_filter_moreinput;
	}
    }
  if (iobuf->osize < 4)
    {
      iobuf->osize = 4;
      return mu_filter_moreoutput;
    }
      
  ptr = (const unsigned char*) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  while (nbytes < osize)
    {
      unsigned char c1 = 0, c2 = 0, x = '=', y = '=';

      if (lp->max_len && lp->cur_len == lp->max_len)
	{
	  if (lp->state == base64_init)
	    lp->idx = 3;
	  lp->state = base64_newline;
	}
      switch (lp->state)
	{
	case base64_init:
	  break;

	case base64_newline:
	  *optr++ = '\n';
	  nbytes++;
	  lp->cur_len = 0;
	  lp->state = base64_rollback;
	  continue;

	case base64_rollback:
	  if (lp->idx < 3)
	    {
	      *optr++ = lp->save[lp->idx++];
	      nbytes++;
	      lp->cur_len++;
	      continue;
	    }
	  lp->state = base64_init;
	}

      if (!(consumed + 3 <= isize || pad))
	break;

      if (consumed == isize)
	{
	  lp->save[1] = x;
	  lp->save[2] = y;
	  lp->idx = 1;
	  lp->state = base64_rollback;
	}
      else
	{
	  *optr++ = b64tab[ptr[0] >> 2];
	  nbytes++;
	  lp->cur_len++;
	  consumed++;
	  switch (isize - consumed)
	    {
	    default:
	      consumed++;
	      y = b64tab[ptr[2] & 0x3f];
	      c2 = ptr[2] >> 6;
	    case 1:
	      consumed++;
	      x = b64tab[((ptr[1] << 2) + c2) & 0x3f];
	      c1 = (ptr[1] >> 4);
	    case 0:
	      lp->save[0] = b64tab[((ptr[0] << 4) + c1) & 0x3f];
	      lp->save[1] = x;
	      lp->save[2] = y;
	      lp->idx = 0;
	      lp->state = base64_rollback;
	    }
	  
	  ptr += 3;
	}
      pad = 0;
    }

  /* Consumed may grow bigger than isize if cmd is mu_filter_lastbuf */
  if (consumed > iobuf->isize)
    consumed = iobuf->isize;
  if (cmd == mu_filter_lastbuf &&
      (consumed < iobuf->isize || lp->state == base64_rollback))
    res = mu_filter_again;
  else
    res = mu_filter_ok;
  iobuf->isize = consumed;
  iobuf->osize = nbytes;
  return res;
}

static int
alloc_state (void **pret, int mode, int argc, const char **argv)
{
  if (mode == MU_FILTER_ENCODE)
    {
      struct base64_line *lp = malloc (sizeof (*lp));
      if (!lp)
	return ENOMEM;
      lp->max_len = 76;
      *pret = lp;
    }
  else
    *pret = NULL;
  return 0;
}

static struct _mu_filter_record _base64_filter = {
  "base64",
  alloc_state,
  _base64_encoder,
  _base64_decoder
};

mu_filter_record_t mu_base64_filter = &_base64_filter;

static struct _mu_filter_record _B_filter = {
  "B",
  NULL,
  _base64_encoder,
  _base64_decoder
};

mu_filter_record_t mu_rfc_2047_B_filter = &_B_filter;
