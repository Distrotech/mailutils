/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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
#include <config.h>
#endif

#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/stream.h>
#include <mailutils/iterator.h>
#include <mailutils/header.h>
#include <mailutils/nls.h>
#include <mailutils/filter.h>
#include <mailutils/property.h>
#include <mailutils/sys/property.h>
#include <stdlib.h>

static void
_mh_prop_done (struct _mu_property *prop)
{
  struct mu_mh_prop *mhprop = prop->_prop_init_data;
  mu_header_t header = prop->_prop_data;
  mu_header_destroy (&header);
  free (mhprop->filename);
  free (mhprop);
}

static int
_mh_prop_getval (struct _mu_property *prop,
		 const char *key, const char **pval)
{
  mu_header_t header = prop->_prop_data;

  if (!header)
    return MU_ERR_NOENT;
  return mu_header_sget_value (header, key, pval);
}

static int
_mh_prop_setval (struct _mu_property *prop, const char *key,
		 const char *val, int overwrite)
{
  struct mu_mh_prop *mhprop = prop->_prop_init_data;
  mu_header_t header = prop->_prop_data;
  if (!header)
    {
      int rc;
      if ((rc = mu_header_create (&header, NULL, 0)) != 0)
	{
	  mu_error (_("cannot create context %s: %s"),
		    mhprop->filename, mu_strerror (rc));
	  return 1;
	}
      prop->_prop_data = header;
    }
  return mu_header_set_value (header, key, val, overwrite);
}

static int
_mh_prop_unset (struct _mu_property *prop, const char *key)
{
  mu_header_t header = prop->_prop_data;
  if (!header)
    return 0;
  return mu_header_remove (header, key, 1);
}

static int
_mh_prop_getitr (struct _mu_property *prop, mu_iterator_t *pitr)
{
  mu_header_t header = prop->_prop_data;
  return mu_header_get_iterator (header, pitr);
}

static int
_mh_prop_clear (struct _mu_property *prop)
{
  mu_header_t header = prop->_prop_data;
  return mu_header_clear (header);
}

    
static int
_mh_prop_read_stream (mu_header_t *phdr, mu_stream_t stream)
{
  int rc;
  mu_stream_t flt;
  const char *argv[4];
  mu_off_t size;
  size_t total;
  char *blurb;
  
  rc = mu_stream_size (stream, &size);
  if (rc)
    return rc;
  
  argv[0] = "INLINE-COMMENT";
  argv[1] = "#";
  argv[2] = "-r";
  argv[3] = NULL;
  rc = mu_filter_create_args (&flt, stream, argv[0], 3, argv,
			      MU_FILTER_DECODE, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("cannot open filter stream: %s"), mu_strerror (rc));
      return rc;
    }

  blurb = malloc (size + 1);
  if (!blurb)
    {
      mu_stream_destroy (&flt);
      return ENOMEM;
    }

  total = 0;
  while (1)
    {
      size_t n;
      
      rc = mu_stream_read (flt, blurb + total, size - total, &n);
      if (rc)
	break;
      if (n == 0)
	break;
      total += n;
    }

  mu_stream_destroy (&flt);
  if (rc)
    {
      free (blurb);
      return rc;
    }

  rc = mu_header_create (phdr, blurb, total);
  free (blurb);
  return rc;
}

static int
_mh_prop_write_stream (mu_header_t header, struct mu_mh_prop *mhprop,
		       mu_stream_t stream)
{
  int rc;
  mu_stream_t instream;
  mu_off_t size;

  mu_header_get_streamref (header, &instream);
  rc = mu_stream_copy (stream, instream, 0, &size);
  if (rc)
    {
      mu_error (_("error writing to context file %s: %s"),
		mhprop->filename, mu_strerror (rc));
      return rc;
    }
  else
    rc = mu_stream_truncate (stream, size);
  mu_stream_destroy (&instream);
  return rc;
}

static int
_mh_prop_fill (struct _mu_property *prop)
{
  struct mu_mh_prop *mhprop = prop->_prop_init_data;
  int rc;
  mu_stream_t stream;
  mu_header_t header;
  
  rc = mu_file_stream_create (&stream, mhprop->filename, MU_STREAM_READ);
  if (rc)
    {
      if ((rc = mu_header_create (&header, NULL, 0)) != 0)
	mu_error (_("cannot create context %s: %s"),
		  mhprop->filename, mu_strerror (rc));
    }
  else
    {
      rc = _mh_prop_read_stream (&header, stream);
      mu_stream_unref (stream);
    }
  if (rc == 0)
    prop->_prop_data = header;
  return rc;
}

static int
_mh_prop_save (struct _mu_property *prop)
{
  struct mu_mh_prop *mhprop = prop->_prop_init_data;
  mu_header_t header = prop->_prop_data;
  mu_stream_t stream;
  int rc;
  
  if (mhprop->ro)
    return 0;
  
  rc = mu_file_stream_create (&stream, mhprop->filename,
			      MU_STREAM_WRITE|MU_STREAM_CREAT);
  if (rc)
    return rc;
  rc = _mh_prop_write_stream (header, mhprop, stream);
  mu_stream_unref (stream);
  return rc;
}

int
mu_mh_property_init (struct _mu_property *prop)
{
  struct mu_mh_prop *mhprop = prop->_prop_init_data;

  if (!mhprop)
    return EINVAL;
  
  prop->_prop_data = NULL;

  prop->_prop_done = _mh_prop_done;
  prop->_prop_fill = _mh_prop_fill;
  prop->_prop_save = _mh_prop_save;
  prop->_prop_getval = _mh_prop_getval;
  prop->_prop_setval = _mh_prop_setval;
  prop->_prop_unset = _mh_prop_unset;
  prop->_prop_getitr = _mh_prop_getitr;
  prop->_prop_clear = _mh_prop_clear;
  return 0;
}
