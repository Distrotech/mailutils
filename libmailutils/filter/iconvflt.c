/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004-2005, 2007, 2010-2012 Free Software Foundation,
   Inc.

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

/* Iconv filter: converts input between two charsets.
   
   This filter operates in decode mode only.  Initialization sequence
   requires two arguments: the name of the input encoding and that
   of the output encoding.  Optional third argument specifies what
   to do when an illegal character sequence is encountered on input.
   Possible values are:
 
     none       - return failure and stop further conversions;
     copy-pass  - copy the offending character to the output verbatim;
     copy-octal - represent it as a C octal sequence (\NNN).

   The default is "copy-octal", unless overridden by mu_default_fallback_mode
   setting (which see).
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mailutils/stream.h>
#include <mailutils/sys/stream.h>
#include <mailutils/filter.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/util.h>

#ifdef HAVE_ICONV_H
# include <iconv.h>
#endif

#ifndef ICONV_CONST
# define ICONV_CONST
#endif

#ifndef HAVE_ICONV
# undef iconv_open
# define iconv_open(tocode, fromcode) ((iconv_t) -1)

# undef iconv
# define iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft) (errno = EILSEQ, (size_t) -1)

# undef iconv_close
# define iconv_close(cd) 0

#endif 

struct _icvt_filter
{
  char *fromcode;
  char *tocode;
  enum mu_iconv_fallback_mode fallback_mode;
  iconv_t cd;           /* Conversion descriptor */
};

static enum mu_filter_result
_icvt_decoder (void *xd,
	       enum mu_filter_command cmd,
	       struct mu_filter_io *iobuf)
{
  struct _icvt_filter *_icvt = xd;
  char *ip, *op;
  size_t ilen, olen;
  int rc;
    
  switch (cmd)
    {
    case mu_filter_init:
      if (mu_c_strcasecmp (_icvt->fromcode, _icvt->tocode) == 0)
	_icvt->cd = (iconv_t) -1;
      else
	{
	  iconv_t cd = iconv_open (_icvt->tocode, _icvt->fromcode);
	  if (cd == (iconv_t) -1)
	    return mu_filter_failure;
	  _icvt->cd = cd;
	}
      return mu_filter_ok;

    case mu_filter_done:
      if (_icvt->cd != (iconv_t) -1)
	iconv_close (_icvt->cd);
      free (_icvt->fromcode);
      free (_icvt->tocode);
      return mu_filter_ok;

    default:
      break;
    }

  if (_icvt->cd == (iconv_t) -1)
    {
      size_t len = iobuf->isize;
      if (len > iobuf->osize)
	len = iobuf->osize;
      memcpy (iobuf->output, iobuf->input, len);
      iobuf->isize = len;
      iobuf->osize = len;
      return mu_filter_ok;
    }
  
  ip = (char*) iobuf->input;
  ilen = iobuf->isize;
  op = iobuf->output;
  olen = iobuf->osize;
 again:
  rc = iconv (_icvt->cd, &ip, &ilen, &op, &olen);
  if (rc == -1)
    {
      switch (errno) 
	{
	case E2BIG:
	  iobuf->osize += 16;
	  return mu_filter_moreoutput;
	
	case EINVAL:
	  /* An incomplete multibyte sequence is encountered in the input */
	  if (ilen == iobuf->isize)
	    {
	      iobuf->isize++;
	      return mu_filter_moreinput;
	    }
	  break;
	  
	case EILSEQ:
	  switch (_icvt->fallback_mode)
	    {
	    case mu_fallback_none:
	      iobuf->errcode = EILSEQ;
	      return mu_filter_failure;
	      
	    case mu_fallback_copy_pass:
	      *op++ = *ip++;
	      ilen--;
	      olen--;
	      break;
	      
	    case mu_fallback_copy_octal:
	      if (mu_isprint (*ip))
		{
		  *op++ = *ip++;
		  ilen--;
		  olen--;
		}
	      else
		{
		  if (olen < 4)
		    {
		      iobuf->osize = 4;
		      return mu_filter_moreoutput;
		    }
		  sprintf (op, "\\%03o", *(unsigned char*)ip);
		  op += 4;
		  olen -= 4;
		  ip++;
		  ilen--;
		}
	    }
	  if (ilen && olen)
	    goto again;
	  break;
	      
	default:
	  iobuf->errcode = errno;
	  return mu_filter_failure;
	}
    }

  iobuf->isize = ip - iobuf->input;
  iobuf->osize = op - iobuf->output;
  return mu_filter_ok;
}

static int
alloc_state (void **pret, int mode MU_ARG_UNUSED, int argc, const char **argv)
{
  struct _icvt_filter *flt;
  const char *from;
  const char *to;
  enum mu_iconv_fallback_mode fallback_mode = mu_default_fallback_mode;
  
  if (argc < 3)
    return EINVAL; /* FIXME: Provide some defaults? */
  if (argc > 4)
    return EINVAL;
  
  from = argv[1];
  to = argv[2];

  if (argc == 4)
    {
      const char *str = argv[3];
      if (strcmp (str, "none") == 0)
	fallback_mode = mu_fallback_none;
      else if (strcmp (str, "copy-pass") == 0)
	fallback_mode = mu_fallback_copy_pass;
      else if (strcmp (str, "copy-octal") == 0)
	fallback_mode = mu_fallback_copy_octal;
      else
	return EINVAL;
    }

  flt = calloc (1, sizeof (*flt));
  if (!flt)
    return ENOMEM;
  flt->fromcode = strdup (from);
  if (!flt->fromcode)
    {
      free (flt);
      return ENOMEM;
    }
  flt->tocode = strdup (to);
  if (!flt->tocode)
    {
      free (flt->fromcode);
      free (flt);
      return ENOMEM;
    }
  flt->fallback_mode = fallback_mode;
  flt->cd = (iconv_t) -1;
  *pret = flt;
  return 0;
}
  
static struct _mu_filter_record _iconv_filter = {
  "ICONV",
  alloc_state,
  _icvt_decoder,
  _icvt_decoder  
};

mu_filter_record_t mu_iconv_filter = &_iconv_filter;
