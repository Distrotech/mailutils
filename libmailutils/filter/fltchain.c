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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <mailutils/filter.h>
#include <mailutils/stream.h>
#include <mailutils/errno.h>

static int
negate_filter_mode (int mode)
{
  if (mode == MU_FILTER_DECODE)
    return MU_FILTER_ENCODE;
  else if (mode == MU_FILTER_ENCODE)
    return MU_FILTER_DECODE;
  abort ();
}

static int
_add_next_link (mu_stream_t *pret, mu_stream_t transport,
		int defmode, int flags,
		size_t argc, char **argv,
		int (*pred) (void *, mu_stream_t, const char *),
		void *closure)
{
  int mode;
  int qmark = 0;
  char *fltname;
  int status = 0;
      
  fltname = argv[0];
  if (fltname[0] == '?')
    {
      if (pred)
	qmark = 1;
      fltname++;
    }
  
  if (fltname[0] == '~')
    {
      mode = negate_filter_mode (defmode);
      fltname++;
    }
  else
    mode = defmode;
      
  if (qmark == 0 || pred (closure, transport, fltname))
    {
      status = mu_filter_create_args (pret, transport, fltname,
				      argc, (const char **)argv,
				      mode, flags);
      mu_stream_unref (transport);
    }
  return status;
}

int
_filter_chain_create (mu_stream_t *pret, mu_stream_t transport,
		      int defmode,
		      int flags,
		      size_t argc, char **argv,
		      int (*pred) (void *, mu_stream_t, const char *),
		      void *closure)
{
  while (argc)
    {
      size_t i;
      int status;
      mu_stream_t stream;

      for (i = 1; i < argc; i++)
	if (strcmp (argv[i], "+") == 0)
	  break;

      status = _add_next_link (&stream, transport,
			       defmode, flags,
			       i, argv,
			       pred, closure);
      if (status)
	return status;
      transport = stream;

      argc -= i;
      argv += i;
      if (argc)
	{
	  argc--;
	  argv++;
	}
    }
  *pret = transport;
  return 0;
}

int
_filter_chain_create_rev (mu_stream_t *pret, mu_stream_t transport,
			  int defmode,
			  int flags,
			  size_t argc, char **argv,
			  int (*pred) (void *, mu_stream_t, const char *),
			  void *closure)
{
  size_t pos;

  for (pos = argc; pos > 0;)
    {
      size_t i;
      int status;
      mu_stream_t stream;

      for (i = pos; i > 0; i--)
	{
	  if (strcmp (argv[i - 1], "+") == 0)
	    break;
	}

      status = _add_next_link (&stream, transport,
			       defmode, flags,
			       pos - i, argv + i,
			       pred, closure);
      if (status)
	return status;
      transport = stream;
      if (i > 0)
	i--;
      pos = i;
    }
  *pret = transport;
  return 0;
}
  
int
mu_filter_chain_create_pred (mu_stream_t *pret, mu_stream_t transport,
			     int defmode,
			     int flags,
			     size_t argc, char **argv,
			     int (*pred) (void *, mu_stream_t, const char *),
			     void *closure)
{
  int rc;
  
  mu_stream_ref (transport);
  if (flags & MU_STREAM_WRITE)
    rc = _filter_chain_create_rev (pret, transport,
				   defmode, flags,
				   argc, argv,
				   pred, closure);
  else 
    rc = _filter_chain_create (pret, transport,
			       defmode, flags,
			       argc, argv, pred, closure);
  if (rc)
    mu_stream_unref (transport);
  return rc;
}

int
mu_filter_chain_create (mu_stream_t *pret, mu_stream_t transport,
			int defmode, int flags,
			size_t argc, char **argv)
{
  return mu_filter_chain_create_pred (pret, transport, defmode, flags,
				      argc, argv, NULL, NULL);
}

