/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#include <unistd.h>
#include <stdlib.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/list.h>
#include <mailutils/iterator.h>
#include <mailutils/nls.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>

struct onexit_closure
{
  mu_onexit_t function;
  void *data;
};

static mu_list_t onexit_list;

void
_mu_onexit_run (void)
{
  mu_iterator_t itr;
  int rc, status = 0;

  if (!onexit_list)
    return;
  rc = mu_list_get_iterator (onexit_list, &itr);
  if (rc)
    {
      mu_error (_("cannot create iterator, onexit aborted: %s"),
		mu_strerror (rc));
      mu_stream_destroy (&mu_strerr);
      _exit (127);
    }
  
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      struct onexit_closure *cp;
      int rc = mu_iterator_current (itr, (void**)&cp);
      if (rc)
	{
	  status = 127;
	  mu_error (_("cannot obtain current item while traversing the"
		      " onexit action list: %s"), mu_strerror (rc));
	}
      else
	cp->function (cp->data);
      mu_iterator_ctl (itr, mu_itrctl_delete, NULL);
    }
  mu_iterator_destroy (&itr);
  mu_list_destroy (&onexit_list);
  if (status)
    _exit (status);
}

int
mu_onexit (mu_onexit_t func, void *data)
{
  struct onexit_closure *clos = malloc (sizeof (*clos));
  if (!clos)
    return ENOMEM;
  clos->function = func;
  clos->data = data;
  if (!onexit_list)
    {
      int rc = mu_list_create (&onexit_list);
      mu_list_set_destroy_item (onexit_list, mu_list_free_item);
      if (rc)
	return rc;
      atexit (_mu_onexit_run);
    }
  return mu_list_append (onexit_list, clos);
}

void
mu_onexit_reset (void)
{
  mu_list_clear (onexit_list);
}

void
mu_onexit_run (void)
{
  _mu_onexit_run ();
}
