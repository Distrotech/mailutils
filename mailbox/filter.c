/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2004 Free Software Foundation, Inc.

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
First draft: Alain Magloire.

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

#include <filter0.h>

#include <mailutils/iterator.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>

static void
filter_destroy (stream_t stream)
{
  filter_t filter = stream_get_owner (stream);
  if (filter->_destroy)
    filter->_destroy (filter);
  if (filter->property)
    property_destroy (&(filter->property), filter);
  free (filter);
}

static int
filter_read (stream_t stream, char *buffer, size_t buflen, off_t offset,
	     size_t *nbytes)
{
  filter_t filter = stream_get_owner (stream);
  if (filter->_read && (filter->direction & MU_STREAM_READ
		      || filter->direction & MU_STREAM_RDWR))
    return filter->_read (filter, buffer, buflen, offset, nbytes);
  return stream_read (filter->stream, buffer, buflen, offset, nbytes);
}

static int
filter_readline (stream_t stream, char *buffer, size_t buflen,
		 off_t offset, size_t *nbytes)
{
  filter_t filter = stream_get_owner (stream);
  if (filter->_readline && (filter->direction & MU_STREAM_READ
			  || filter->direction & MU_STREAM_RDWR))
    return filter->_readline (filter, buffer, buflen, offset, nbytes);
  return stream_readline (filter->stream, buffer, buflen, offset, nbytes);
}

static int
filter_write (stream_t stream, const char *buffer, size_t buflen,
	      off_t offset, size_t *nbytes)
{
  filter_t filter = stream_get_owner (stream);
  if (filter->_write && (filter->direction & MU_STREAM_WRITE
		       || filter->direction & MU_STREAM_RDWR))
    return filter->_write (filter, buffer, buflen, offset, nbytes);
  return stream_write (filter->stream, buffer, buflen, offset, nbytes);
}

static int
filter_open (stream_t stream)
{
  filter_t filter = stream_get_owner (stream);

  return stream_open (filter->stream);
}

static int
filter_truncate (stream_t stream, off_t len)
{
  filter_t filter = stream_get_owner (stream);
  return stream_truncate (filter->stream, len);
}

static int
filter_size (stream_t stream, off_t *psize)
{
  filter_t filter = stream_get_owner (stream);
  return stream_size (filter->stream, psize);
}

static int
filter_flush (stream_t stream)
{
  filter_t filter = stream_get_owner(stream);
  return stream_flush (filter->stream);
}

static int
filter_get_transport2 (stream_t stream, mu_transport_t *pin, mu_transport_t *pout)
{
  filter_t filter = stream_get_owner (stream);
  return stream_get_transport2 (filter->stream, pin, pout);
}

static int
filter_close (stream_t stream)
{
  filter_t filter = stream_get_owner (stream);
  return stream_close (filter->stream);
}

/* NOTE: We will leak here since the monitor of the filter will never
   be release.  That's ok we can leave with this, it's only done once.  */
static list_t filter_list;
struct _monitor filter_monitor = MU_MONITOR_INITIALIZER;

int
filter_get_list (list_t *plist)
{
  if (plist == NULL)
    return MU_ERR_OUT_PTR_NULL;
  monitor_wrlock (&filter_monitor);
  if (filter_list == NULL)
    {
      int status = list_create (&filter_list);
      if (status != 0)
	return status;
      /* Default filters.  */
      list_append (filter_list, base64_filter);
      list_append (filter_list, qp_filter);
      list_append (filter_list, binary_filter);
      list_append (filter_list, bit8_filter);
      list_append (filter_list, bit7_filter);
      list_append (filter_list, rfc822_filter);
      list_append (filter_list, rfc_2047_Q_filter);
      /* FIXME: add the default encodings?  */
    }
  *plist = filter_list;
  monitor_unlock (&filter_monitor);
  return 0;
}

int
filter_create (stream_t *pstream, stream_t stream, const char *name,
	       int type, int direction)
{
  iterator_t iterator = NULL;
  filter_record_t filter_record = NULL;
  int  (*f_init)  __P ((filter_t))  = NULL;
  int found = 0;
  int status;
  list_t list = NULL;

  if (pstream == NULL)
    return MU_ERR_OUT_PTR_NULL;
  if (stream == NULL || name == NULL)
    return EINVAL;

  filter_get_list (&list);
  status = list_get_iterator (list, &iterator);
  if (status != 0)
    return status;

  for (iterator_first (iterator); !iterator_is_done (iterator);
       iterator_next (iterator))
    {
      iterator_current (iterator, (void **)&filter_record);
      if ((filter_record->_is_filter
	   && filter_record->_is_filter (filter_record, name))
	  || (strcasecmp (filter_record->name, name) == 0))
        {
	  found = 1;
	  if (filter_record->_get_filter)
	    filter_record->_get_filter (filter_record, &f_init);
	  else
	    f_init = filter_record->_filter;
	  break;
        }
    }
  iterator_destroy (&iterator);

  if (found)
    {
      int flags = 0;
      filter_t filter;

      filter = calloc (1, sizeof (*filter));
      if (filter == NULL)
	return ENOMEM;

      stream_get_flags (stream, &flags);
      status = stream_create (pstream, flags | MU_STREAM_NO_CHECK, filter);
      if (status != 0)
	{
	  free (filter);
	  return status;
	}

      filter->stream = stream;
      filter->filter_stream = *pstream;
      filter->direction = (direction == 0) ? MU_STREAM_READ : direction;
      filter->type = type;

      status = property_create (&(filter->property), filter);
      if (status != 0)
	{
	  stream_destroy (pstream, filter);
	  free (filter);
	  return status;
	}
      property_set_value (filter->property, "DIRECTION",
			  ((filter->direction == MU_STREAM_WRITE) ? "WRITE":
			   (filter->direction == MU_STREAM_RDWR) ? "RDWR" :
			   "READ"), 1);
      property_set_value (filter->property, "TYPE", filter_record->name, 1);
      stream_set_property (*pstream, filter->property, filter);

      if (f_init != NULL)
	{
	  status = f_init (filter);
	  if (status != 0)
	    {
	      stream_destroy (pstream, filter);
	      free (filter);
	      return status;
	    }
        }

      stream_set_open (*pstream, filter_open, filter );
      stream_set_close (*pstream, filter_close, filter );
      stream_set_read (*pstream, filter_read, filter);
      stream_set_readline (*pstream, filter_readline, filter);
      stream_set_write (*pstream, filter_write, filter);
      stream_set_get_transport2 (*pstream, filter_get_transport2, filter );
      stream_set_truncate (*pstream, filter_truncate, filter );
      stream_set_size (*pstream, filter_size, filter );
      stream_set_flush (*pstream, filter_flush, filter );
      stream_set_destroy (*pstream, filter_destroy, filter);
    }
  else
    status = MU_ERR_NOENT;
  return status;
}
