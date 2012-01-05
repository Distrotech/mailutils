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

/* This file implements a DOT filter, useful for data I/O in
   such protocols as POP3 and SMTP.  When encoding, this filter
   "byte-stuffs" the input by outputting an additional '.' in front
   of any '.' appearing at the beginning of a line.  Upon closing the
   filter in this mode, it outputs additional ".\n".
   
   When decoding, the reverse is performed: any '.' appearing at the
   beginning of a line is removed.  A single dot on a line by itself
   marks end of the stream.

   MU also provides a hairy version of this filter, called CRLFDOT.
   In addition to byte-stuffing, CRLFDOT also performs CRLF/LF translation.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <mailutils/errno.h>
#include <mailutils/filter.h>
#include <mailutils/stream.h>

enum dot_decode_state
  {
    dot_decode_init,  /* initial state */
    dot_decode_char,  /* Any character excepting [\r\n.] */
    dot_decode_lf,    /* prev. char was \n */
    dot_decode_dot,   /* 2 prev. chars were \n. */
    dot_decode_end   /* final state, a \n.\n seen. */
  };

static enum dot_decode_state
new_decode_state (enum dot_decode_state state, int c)
{
  switch (state)
    {
    case dot_decode_init:
      switch (c)
	{
	case '.':
	  return dot_decode_dot;
	}
      break;
      
    case dot_decode_char:
      switch (c)
	{
	case '\n':
	  return dot_decode_lf;
	}
      break;
      
    case dot_decode_lf:
      switch (c)
	{
	case '.':
	  return dot_decode_dot;
	case '\n':
	  return dot_decode_lf;
	}
      break;
      
    case dot_decode_dot:
      switch (c)
	{
	case '\n':
	  return dot_decode_end;
	}
      break;

    case dot_decode_end:
      return dot_decode_end;
    }
  return dot_decode_char;
}

/* Move min(isize,osize) bytes from iptr to optr, replacing each \r\n
   with \n. */
static enum mu_filter_result
_dot_decoder (void *xd,
	      enum mu_filter_command cmd,
	      struct mu_filter_io *iobuf)
{
  int *pstate = xd;
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;

  switch (cmd)
    {
    case mu_filter_init:
      *pstate = dot_decode_init;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;
      
    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; *pstate != dot_decode_end && i < isize && j < osize; i++)
    {
      unsigned char c = *iptr++;
      int curstate = *pstate;
      
      *pstate = new_decode_state (curstate, c);
      if (!(c == '.' && (curstate == dot_decode_init ||
			 curstate == dot_decode_lf)))
	optr[j++] = c;
    }
  
  if (*pstate == dot_decode_end)
    iobuf->eof = 1;

  iobuf->isize = i;
  iobuf->osize = j;
  return mu_filter_ok;
}

enum dot_encode_state
  {
    dot_encode_init,  /* initial state */
    dot_encode_char,  /* Any character excepting \n */
    dot_encode_lf,    /* prev. char was \n */
  };    

static enum dot_encode_state
new_encode_state (int c)
{
  switch (c)
    {
    case '\n':
      return dot_encode_lf;
    }
  return dot_encode_char;
}

/* Move min(isize,osize) bytes from iptr to optr, replacing each \n
   with \r\n.  Any input \r\n sequences remain untouched. */
static enum mu_filter_result
_dot_encoder (void *xd,
	      enum mu_filter_command cmd,
	      struct mu_filter_io *iobuf)
{
  enum mu_filter_result result;
  size_t i, j;
  const unsigned char *iptr;
  size_t isize;
  char *optr;
  size_t osize;
  int *state = xd;
  
  switch (cmd)
    {
    case mu_filter_init:
      *state = dot_encode_init;
      return mu_filter_ok;
      
    case mu_filter_done:
      return mu_filter_ok;

    default:
      break;
    }
  
  iptr = (const unsigned char *) iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  for (i = j = 0; i < isize && j < osize; i++, iptr++)
    {
      unsigned char c = *iptr;
      int curstate = *state;

      if (c == '.' && (curstate == dot_encode_init ||
		       curstate == dot_encode_lf))
	{
 	  if (j + 2 > osize)
	    {
	      if (i == 0)
		{
		  iobuf->osize = 2;
		  return mu_filter_moreoutput;
		}
	      break;
	    }
	  optr[j++] = '.';
	  optr[j++] = '.';
	}
      else
	optr[j++] = c;

      *state = new_encode_state (c);
    }

  result = mu_filter_ok;
  if (cmd == mu_filter_lastbuf)
    {
      switch (*state)
	{
	case dot_encode_lf:
	  if (j + 2 > osize)
	    result = mu_filter_again;
	  break;
	      
	default:
	  if (j + 3 > osize)
	    result = mu_filter_again;
	  else
	    optr[j++] = '\n';
	}

      if (result == mu_filter_ok)
	{
	  optr[j++] = '.';
	  optr[j++] = '\n';
	}
    }

  iobuf->isize = i;
  iobuf->osize = j;
  return result;
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED,
	     int argc MU_ARG_UNUSED, const char **argv MU_ARG_UNUSED)
{
  *pret = malloc (sizeof (int));
  if (!*pret)
    return ENOMEM;
  return 0;
}

static struct _mu_filter_record _dot_filter = {
  "DOT",
  alloc_state,
  _dot_encoder,
  _dot_decoder
};

mu_filter_record_t mu_dot_filter = &_dot_filter;
