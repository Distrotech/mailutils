/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2007, 2009, 2010 Free
   Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

/* Notes:
   First draft: Alain Magloire.
   Complete rewrite: Sergey Poznyakoff.
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif

#include <mailutils/filter.h>
#include <mailutils/monitor.h>
#include <mailutils/list.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>

/* NOTE: We will leak here since the monitor of the filter will never
   be release.  That's ok we can leave with this, it's only done once.  */
static mu_list_t filter_list;
struct mu_monitor filter_monitor = MU_MONITOR_INITIALIZER;

static int
filter_name_cmp (const void *item, const void *data)
{
  struct _mu_filter_record const *rec = item;
  char const *name = data;
  return mu_c_strcasecmp (rec->name, name);
}

int
mu_filter_get_list (mu_list_t *plist)
{
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;
  mu_monitor_wrlock (&filter_monitor);
  if (filter_list == NULL)
    {
      int status = mu_list_create (&filter_list);
      if (status != 0)
	return status;
      mu_list_set_comparator (filter_list, filter_name_cmp);
      /* Default filters.  */
      mu_list_append (filter_list, mu_base64_filter);
      mu_list_append (filter_list, mu_qp_filter);
      mu_list_append (filter_list, mu_binary_filter);
      mu_list_append (filter_list, mu_bit8_filter);
      mu_list_append (filter_list, mu_bit7_filter);
      mu_list_append (filter_list, mu_rfc822_filter);
      mu_list_append (filter_list, mu_crlf_filter);
      mu_list_append (filter_list, mu_crlfdot_filter);
      mu_list_append (filter_list, mu_rfc_2047_Q_filter);
      mu_list_append (filter_list, mu_rfc_2047_B_filter);
      /* FIXME: add the default encodings?  */
    }
  *plist = filter_list;
  mu_monitor_unlock (&filter_monitor);
  return 0;
}

static int
filter_create_rd (mu_stream_t *pstream, mu_stream_t stream,
		  size_t max_line_length,
		  int mode,
		  mu_filter_xcode_t xcode, void *xdata,
		  int flags)
{
  int status;
  mu_stream_t fltstream;

  flags &= ~MU_STREAM_AUTOCLOSE;
  
  status = mu_filter_stream_create (&fltstream, stream,
				    mode, xcode, xdata,
				    flags);
  if (status == 0)
    {
      if (max_line_length)
	{
	  status = mu_linelen_filter_create (pstream, fltstream,
					     max_line_length,
					     flags);
	  mu_stream_unref (fltstream);
	  if (status)
	    return status;
	}
      else
	*pstream = fltstream;

      if (flags & MU_STREAM_AUTOCLOSE)
	mu_stream_unref (stream);
    }
  return status;
}

static int
filter_create_wr (mu_stream_t *pstream, mu_stream_t stream,
		  size_t max_line_length,
		  int mode,
		  mu_filter_xcode_t xcode, void *xdata,
		  int flags)
{
  int status;
  mu_stream_t fltstream, instream = NULL, tmpstr;

  flags &= ~MU_STREAM_AUTOCLOSE;

  if (max_line_length)
    {
      status = mu_linelen_filter_create (&instream, stream,
					 max_line_length,
					 flags);
      if (status)
	return status;
      tmpstr = instream;
    }
  else
    tmpstr = stream;
  
  status = mu_filter_stream_create (&fltstream, tmpstr,
				    mode, xcode, xdata,
				    flags);
  mu_stream_unref (instream);
  if (status)
    return status;
  *pstream = fltstream;
  if (flags & MU_STREAM_AUTOCLOSE)
    mu_stream_unref (stream);
  return status;
}

int
mu_filter_create (mu_stream_t *pstream, mu_stream_t stream, const char *name,
		  int mode, int flags)
{
  int status;
  mu_filter_record_t frec;
  mu_list_t list;
  void *xdata = NULL;
  
  if ((flags & MU_STREAM_RDWR) == MU_STREAM_RDWR)
    return EINVAL;
  
  mu_filter_get_list (&list);
  status = mu_list_locate (list, (void*)name, (void**)&frec);
  if (status)
    return status;

  if (frec->newdata)
    {
      status = frec->newdata (&xdata, mode, NULL);
      if (status)
	return status;
    }

  status = ((flags & MU_STREAM_WRITE) ? filter_create_wr : filter_create_rd)
                   (pstream, stream,
		    frec->max_line_length,
		    mode,
		    mode == MU_FILTER_ENCODE ? frec->encoder : frec->decoder,
		    xdata,
		    flags);
  if (status)
    free (xdata);
  return status;
}
