/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <mailutils/assoc.h>
#include <mailutils/header.h>
#include <mailutils/message.h>
#include <mailutils/mime.h>
#include <mailutils/iterator.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/util.h>
#include <mailutils/cstr.h>
#include <mailutils/cctype.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>

struct named_param
{
  const char *name;
  struct mu_mime_param const *param;
};

static int
sort_names (void const *a, void const *b)
{
  struct named_param const *pa = a;
  struct named_param const *pb = b;
  return mu_c_strcasecmp (pa->name, pb->name);
}

static int
print_named_param (void *item, void *data)
{
  struct named_param const *p = item;
  struct mu_mime_param const *param = p->param;
  
  mu_printf ("%s", p->name);
  if (param->lang)
    mu_printf ("(lang:%s/%s)", param->lang, param->cset);
  mu_printf ("=%s\n", param->value);
  return 0;
}

int
main (int argc, char **argv)
{
  int i;
  mu_stream_t tmp;
  mu_transport_t trans[2];
  char *value;
  mu_assoc_t assoc;
  mu_iterator_t itr;
  mu_list_t list;
  char *charset = NULL;
  
  mu_set_program_name (argv[0]);
  for (i = 1; i < argc; i++)
    {
      char *opt = argv[i];

      if (strncmp (opt, "-debug=", 7) == 0)
	mu_debug_parse_spec (opt + 7);
      else if (strncmp (opt, "-charset=", 9) == 0)
	charset = opt + 9;
      else if (strcmp (opt, "-h") == 0 || strcmp (opt, "-help") == 0)
	{
	  mu_printf ("usage: %s [-charset=cs] [-debug=SPEC]", mu_program_name);
	  return 0;
	}
      else
	{
	  mu_error ("unknown option %s", opt);
	  return 1;
	}
    }

  if (i != argc)
    {
      mu_error ("too many arguments");
      return 1;
    }
	    
  MU_ASSERT (mu_memory_stream_create (&tmp, MU_STREAM_RDWR));
  MU_ASSERT (mu_stream_copy (tmp, mu_strin, 0, NULL));
  MU_ASSERT (mu_stream_write (tmp, "", 1, NULL));
  MU_ASSERT (mu_stream_ioctl (tmp, MU_IOCTL_TRANSPORT, MU_IOCTL_OP_GET,
			      trans));
      
  MU_ASSERT (mu_mime_header_parse ((char*)trans[0], charset, &value, &assoc));

  mu_printf ("%s\n", value);
  MU_ASSERT (mu_list_create (&list));
  MU_ASSERT (mu_assoc_get_iterator (assoc, &itr));
  for (mu_iterator_first (itr); !mu_iterator_is_done (itr);
       mu_iterator_next (itr))
    {
      const char *name;
      struct mu_mime_param *p;
      struct named_param *np;
      
      mu_iterator_current_kv (itr, (const void **)&name, (void**)&p);
      np = malloc (sizeof (*np));
      if (!np)
	abort ();
      np->name = name;
      np->param = p;
      MU_ASSERT (mu_list_append (list, np));
    }
  mu_iterator_destroy (&itr);

  mu_list_sort (list, sort_names);
  mu_list_foreach (list, print_named_param, NULL);
  
  return 0;
}
  
