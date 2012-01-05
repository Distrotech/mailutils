/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010-2012 Free Software Foundation, Inc.

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

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/stream.h>

enum from_decode_state
  {
    from_decode_init,
    from_decode_nl,
    from_decode_char
  };

#define GT_FROM_MARK_STR ">From "
#define GT_FROM_MARK_LEN (sizeof (GT_FROM_MARK_STR) - 1)

/* Move min(isize,osize) bytes from iptr to optr, replacing each '>From '
   at the beginning of line with 'From '. */
static enum mu_filter_result
_from_decoder (void *xd,
	       enum mu_filter_command cmd,
	       struct mu_filter_io *iobuf)
{
  int *pstate = xd;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;
  enum from_decode_state state;
  size_t i, j;

  switch (cmd)
    {
    case mu_filter_init:
      *pstate = from_decode_init;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      state = *pstate;
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      
      if (c == '\n')
	state = from_decode_nl;
      else if (state == from_decode_init || state == from_decode_nl)
	{
	  size_t len = isize - i;

	  if (len < GT_FROM_MARK_LEN)
	    {
	      if (memcmp (iptr - 1, GT_FROM_MARK_STR, len) == 0)
		{
		  if (i == 0)
		    {
		      iobuf->isize = GT_FROM_MARK_LEN - len;
		      return mu_filter_moreinput;
		    }
		  break;
		}
	      else
		state = from_decode_char;
	    }
	  else if (memcmp (iptr - 1, GT_FROM_MARK_STR, GT_FROM_MARK_LEN) == 0)
	    {
	      /* Skip > */
	      state = from_decode_char;
	      continue;
	    }
	}	  
      optr[j++] = c;
    }

  *pstate = state;
  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

#define FROM_MARK_STR "From "
#define FROM_MARK_LEN (sizeof (FROM_MARK_STR) - 1)

enum from_encode_state
  {
    from_encode_init,
    from_encode_nl,
    from_encode_char,
    from_encode_gt,
    from_encode_f,
    from_encode_r,
    from_encode_o,
    from_encode_m,
    from_encode_sp
  };

static int length_to_state_tab[] = {
  from_encode_gt,
  from_encode_f,
  from_encode_r,
  from_encode_o,
  from_encode_m,
  from_encode_sp
};

static int state_to_length_tab[] = {
  0, 0, 0,
  GT_FROM_MARK_LEN,
  GT_FROM_MARK_LEN-1,
  GT_FROM_MARK_LEN-2,
  GT_FROM_MARK_LEN-3,
  GT_FROM_MARK_LEN-4,
  GT_FROM_MARK_LEN-5
};      

/* Move min(isize,osize) bytes from iptr to optr, replacing each 'From '
   at the beginning of line with '>From '. */

static enum mu_filter_result
_from_encoder (void *xd,
	       enum mu_filter_command cmd,
	       struct mu_filter_io *iobuf)
{
  int *pstate = xd;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;
  enum from_encode_state state;
  size_t i, j;
  
  switch (cmd)
    {
    case mu_filter_init:
      *pstate = from_encode_init;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      state = *pstate;
      switch (state)
	{
	case from_encode_init:
	case from_encode_nl:
	case from_encode_char:
	  break;

	default:
	  osize = state_to_length_tab[state];
	  if (iobuf->osize < osize)
	    {
	      iobuf->osize = osize;
	      return mu_filter_moreoutput;
	    }
	  memcpy (iobuf->output, GT_FROM_MARK_STR + GT_FROM_MARK_LEN - osize,
		  osize);
	  iobuf->osize = osize;
	  iobuf->isize = osize;
	  *pstate = from_encode_init;
	  return mu_filter_ok;
	}
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      
      if (c == '\n')
	state = from_encode_nl;
      else if (state == from_encode_init || state == from_encode_nl)
	{
	  size_t len = isize - i;

	  if (len < FROM_MARK_LEN)
	    {
	      if (memcmp (iptr - 1, FROM_MARK_STR, len) == 0)
		{
		  if (i == 0)
		    {
		      iobuf->isize = FROM_MARK_LEN;
		      return mu_filter_moreinput;
		    }
		  break;
		}
	      else
		state = from_encode_char;
	    }
	  else if (memcmp (iptr - 1, FROM_MARK_STR, FROM_MARK_LEN) == 0)
	    {
	      size_t rest = osize - j;
	      
	      if (rest > GT_FROM_MARK_LEN)
		rest = GT_FROM_MARK_LEN;
	      else if (rest < 2)
		{
		  if (i == 0)
		    {
		      iobuf->osize = GT_FROM_MARK_LEN;
		      return mu_filter_moreoutput;
		    }
		  break;
		}
	      
	      memcpy (optr + j, GT_FROM_MARK_STR, rest);
	      i += rest - 2;
	      iptr += rest - 2;
	      j += rest;
	      if (rest < GT_FROM_MARK_LEN)
		state = length_to_state_tab[rest];
	      else
		state = from_encode_char;
	      continue;
	    }
	  else
	    state = from_encode_char;
	}
      optr[j++] = c;
    }
  *pstate = state;
  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

static int
_from_alloc_state (void **pret, int mode,
		   int argc MU_ARG_UNUSED, const char **argv MU_ARG_UNUSED)
{
  *pret = malloc (sizeof (int));
  if (!*pret)
    return ENOMEM;
  return 0;
}

static struct _mu_filter_record _from_filter = {
  "FROM",
  _from_alloc_state,
  _from_encoder,
  _from_decoder
};

mu_filter_record_t mu_from_filter = &_from_filter;

