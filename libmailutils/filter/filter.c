/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004-2005, 2007, 2009-2012 Free Software
   Foundation, Inc.

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
      mu_list_append (filter_list, mu_dot_filter);
      mu_list_append (filter_list, mu_rfc_2047_Q_filter);
      mu_list_append (filter_list, mu_rfc_2047_B_filter);
      mu_list_append (filter_list, mu_from_filter);
      mu_list_append (filter_list, mu_inline_comment_filter);
      mu_list_append (filter_list, mu_header_filter);
      mu_list_append (filter_list, mu_linecon_filter);
      mu_list_append (filter_list, mu_linelen_filter);
      mu_list_append (filter_list, mu_iconv_filter);
      mu_list_append (filter_list, mu_c_escape_filter);
      /* FIXME: add the default encodings?  */
    }
  *plist = filter_list;
  mu_monitor_unlock (&filter_monitor);
  return 0;
}

int
mu_filter_create_args (mu_stream_t *pstream, mu_stream_t stream,
		       const char *name, int argc, const char **argv,
		       int mode, int flags)
{
  int status;
  mu_filter_record_t frec;
  mu_list_t list;
  void *xdata = NULL;
  mu_filter_xcode_t xcode;
  
  if ((flags & MU_STREAM_RDWR) == MU_STREAM_RDWR)
    return EINVAL;
  
  mu_filter_get_list (&list);
  status = mu_list_locate (list, (void*)name, (void**)&frec);
  if (status)
    return status;

  xcode = mode == MU_FILTER_ENCODE ? frec->encoder : frec->decoder;
  if (!xcode)
    return MU_ERR_EMPTY_VFN;
  
  if (frec->newdata)
    {
      status = frec->newdata (&xdata, mode, argc, argv);
      if (status)
	return status;
    }

  status = mu_filter_stream_create (pstream, stream,
				    mode, xcode, xdata,
				    flags);
  if (status)
    free (xdata);  

  return status;
}

int
mu_filter_create (mu_stream_t *pstream, mu_stream_t stream, const char *name,
		  int mode, int flags)
{
  const char *argv[2];
  argv[0] = name;
  argv[1] = NULL;
  return mu_filter_create_args (pstream, stream, name, 1, argv, mode,
				flags);
}

