/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/alloc.h>
#include <mailutils/stream.h>
#include <mailutils/nls.h>
#include "mu.h"
#include "mu-setup.h"

struct mutool_action_tab
{
  const char *name;
  mutool_action_t action;
  const char *docstring;
};

struct mutool_action_tab mutool_action_tab[] = {
#include "mu-setup.c"
  { NULL }
};

mutool_action_t
dispatch_find_action (const char *name)
{
  struct mutool_action_tab *p;

  for (p = mutool_action_tab; p->name; p++)
    if (strcmp (p->name, name) == 0)
      return p->action;
  return NULL;
}

char *
dispatch_docstring (const char *text)
{
  mu_stream_t str;
  struct mutool_action_tab *p;
  mu_off_t size;
  size_t n;
  char *ret;
  
  mu_memory_stream_create (&str, MU_STREAM_RDWR);
  mu_stream_printf (str, "%s\n%s\n\n", text, _("Commands are:"));
  for (p = mutool_action_tab; p->name; p++)
    mu_stream_printf (str, "  mu %-16s - %s\n",
		      p->name, gettext (p->docstring));
  mu_stream_printf (str, "\n%s\n\n",
		    _("Try `mu COMMAND --help' to get help on a particular "
		      "COMMAND."));
  mu_stream_printf (str, "%s\n", _("Options are:"));
  mu_stream_flush (str);
  mu_stream_size (str, &size);
  ret = mu_alloc (size + 1);
  mu_stream_seek (str, 0, MU_SEEK_SET, NULL);
  mu_stream_read (str, ret, size, &n);
  ret[n] = 0;
  mu_stream_destroy (&str);
  return ret;
}
    
