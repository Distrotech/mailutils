/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

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

#define ISWS(c) ((c)==' ' || (c)=='\t')

static enum mu_filter_result
_qp_decoder (void *xd,
	     enum mu_filter_command cmd,
	     struct mu_filter_io *iobuf)
{
  char c;
  size_t consumed = 0;
  size_t wscount = 0;
  size_t nbytes = 0;
  const char *iptr;
  size_t isize;
  char *optr;
  size_t osize;
  int underscore_special = *(int*)xd;
  
  switch (cmd)
    {
    case mu_filter_init:
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }
  
  iptr = iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;

  while (consumed < isize && nbytes < osize)
    {
      c = *iptr++;

      if (ISWS(c))
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

		  if (nbytes + wscount > osize)
		    sz = osize - nbytes;
		  else
		    sz = wscount;
		  memcpy(optr, iptr - wscount - 1, sz);
		  optr += sz;
		  nbytes += sz;
		  if (wscount > sz)
		    {
		      wscount -= sz;
		      break;
		    }
		}
	      wscount = 0;
	      if (nbytes == osize)
		break;
	    }
	  
	  if (c == '=')
	    {
	      /* There must be 2 more characters before I consume this.  */
	      if (consumed + 2 >= isize)
		break;
	      else
		{
		  /* you get =XX where XX are hex characters.  */
		  char chr[3];
		  int new_c;
		  
		  chr[2] = 0;
		  chr[0] = *iptr++;
		  /* Ignore LF.  */
		  if (chr[0] != '\n')
		    {
		      chr[1] = *iptr++;
		      new_c = strtoul (chr, NULL, 16);
		      *optr++ = new_c;
		      nbytes++;
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
		{
		  if (cmd == mu_filter_lastbuf)
		    consumed++;
		  break;
		}
	      else
		{
		  iptr++; /* Skip the CR character.  */
		  *optr++ = '\n';
		  nbytes++;
		  consumed += 2;
		}
	    }
	  else if (underscore_special && c == '_')
	    {
	      *optr++ = ' ';
	      nbytes++;
	      consumed++;
	    }
	  else
	    {
	      *optr++ = c;
	      nbytes++;
	      consumed++;
	    }
	}	  
    }
  iobuf->isize = consumed - wscount;
  iobuf->osize = nbytes;
  return mu_filter_ok;
}

static enum mu_filter_result
_qp_encoder (void *xd,
	     enum mu_filter_command cmd,
	     struct mu_filter_io *iobuf)
{
  unsigned int c;
  size_t consumed = 0;
  size_t nbytes = 0;
  static const char _hexdigits[] = "0123456789ABCDEF";
  const char *iptr;
  size_t isize;
  char *optr;
  size_t osize;
  int underscore_special = *(int*)xd;

  switch (cmd)
    {
    case mu_filter_init:
    case mu_filter_done:
      return mu_filter_ok;
    default:
      break;
    }

  iptr = iobuf->input;
  isize = iobuf->isize;
  optr = iobuf->output;
  osize = iobuf->osize;
  nbytes = 0;

  /* Strategy: check if we have enough room in the output buffer only
     once the required size has been computed. If there is not enough,
     return and hope that the caller will free up the output buffer a
     bit. */
  
  while (consumed < isize)
    {
      int simple_char;
      
      /* candidate byte to convert */
      c = *(unsigned char*) iptr;
      if (underscore_special && c == '_')
	simple_char = 0;
      else
	simple_char = (c >= 32 && c <= 60)
	               || (c >= 62 && c <= 126)
	               || c == '\t'
	               || c == '\n';

      if (simple_char)
	{
	  /* a non-quoted character uses up one byte */
	  if (nbytes + 1 > osize) 
	    break;
	  if (underscore_special && c == ' ')
	    *optr++ = '_';
	  else
	    *optr++ = c;
	  nbytes++;
	    
	  iptr++;
	  consumed++;

	}
      else
	{
	  /* a quoted character uses up three bytes */
	  if (nbytes + 3 > osize) 
	    break;

	  *optr++ = '=';
	  *optr++ = _hexdigits[(c >> 4) & 0xf];
	  *optr++ = _hexdigits[c & 0xf];
	  
	  nbytes += 3;
	  
	  /* we've actuall used up one byte of input */
	  iptr++;
	  consumed++;
	}
    }
  iobuf->isize = consumed;
  iobuf->osize = nbytes;
  return mu_filter_ok;
}

static int
alloc_qp (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  int *x = malloc (sizeof (*x));
  if (!x)
    return ENOMEM;
  *x = 0;
  *pret = x;
  return 0;
}

static struct _mu_filter_record _qp_filter = {
  "quoted-printable",
  alloc_qp,
  _qp_encoder,
  _qp_decoder
};

mu_filter_record_t mu_qp_filter = &_qp_filter;

static int
alloc_Q (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  int *x = malloc (sizeof (*x));
  if (!x)
    return ENOMEM;
  *x = 1;
  *pret = x;
  return 0;
}

static struct _mu_filter_record _Q_filter = {
  "Q",
  alloc_Q,
  _qp_encoder,
  _qp_decoder
};

mu_filter_record_t mu_rfc_2047_Q_filter = &_Q_filter;
