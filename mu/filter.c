/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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
#include <mailutils/mailutils.h>
#include "mu.h"

char filter_docstring[] = N_("apply a chain of filters to the input");
static char filter_args_doc[] = N_("[~]NAME [ARGS] [+ [~]NAME [ARGS]...]");

static int filter_mode = MU_FILTER_ENCODE;
static int newline_option = 0;
static int list_option;

static void
set_encode_mode (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  filter_mode = MU_FILTER_ENCODE;
}

static void
set_decode_mode (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  filter_mode = MU_FILTER_DECODE;
}

static struct mu_option filter_options[] = {
  { "encode", 'e', NULL, MU_OPTION_DEFAULT,
    N_("encode the input (default)"),
    mu_c_string, NULL, set_encode_mode },
  { "decode", 'd', NULL, MU_OPTION_DEFAULT,
    N_("decode the input"),
    mu_c_string, NULL, set_decode_mode },
  { "newline", 'n', NULL, MU_OPTION_DEFAULT,
    N_("print additional newline"),
    mu_c_bool, &newline_option },
  { "list", 'L', NULL, MU_OPTION_DEFAULT,
    N_("list supported filters"),
    mu_c_bool, &list_option },
  MU_OPTION_END
};

static int
filter_printer (void *item, void *data)
{
  mu_filter_record_t rec = item;
  printf ("%s\n", rec->name);
  return 0;
}

static int
list_filters (void)
{
  mu_list_t list;
  int rc = mu_filter_get_list (&list);

  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_filter_get_list", NULL, rc);
      return 1;
    }
  return mu_list_foreach (list, filter_printer, NULL);
}

static int
negate_filter_mode (int mode)
{
  if (mode == MU_FILTER_DECODE)
    return MU_FILTER_ENCODE;
  else if (mode == MU_FILTER_ENCODE)
    return MU_FILTER_DECODE;
  abort ();
}

int
mutool_filter (int argc, char **argv)
{
  int rc;
  mu_stream_t flt, prev_stream;
  const char *fltname;
  int mode;

  mu_action_getopt (&argc, &argv, filter_options, filter_docstring,
		    filter_args_doc);
  
  if (list_option)
    {
      if (argc)
	{
	  mu_error (_("excess arguments"));
	  return 1;
	}
      return list_filters ();
    }
  
  if (argc == 0)
    {
      mu_error (_("what filter do you want?"));
      return 1;
    }

  prev_stream = mu_strin;
  mu_stream_ref (mu_strin);
  do
    {
      int i;
      
      fltname = argv[0];
      if (fltname[0] == '~')
	{
	  mode = negate_filter_mode (filter_mode);
	  fltname++;
	}
      else
	mode = filter_mode;

      for (i = 1; i < argc; i++)
	if (strcmp (argv[i], "+") == 0)
	  break;
      
      rc = mu_filter_create_args (&flt, prev_stream, fltname,
				  i, (const char **)argv,
				  mode, MU_STREAM_READ);
      mu_stream_unref (prev_stream);
      if (rc)
	{
	  mu_error (_("cannot open filter stream: %s"), mu_strerror (rc));
	  return 1;
	}
      prev_stream = flt;
      argc -= i;
      argv += i;
      if (argc)
	{
	  argc--;
	  argv++;
	}
    }
  while (argc);
  
  rc = mu_stream_copy (mu_strout, flt, 0, NULL);

  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return 1;
    }

  if (newline_option)
    mu_stream_write (mu_strout, "\n", 1, NULL);

  mu_stream_destroy (&flt);
  mu_stream_flush (mu_strout);
  
  return 0;
}

/*
  MU Setup: filter
  mu-handler: mutool_filter
  mu-docstring: filter_docstring
  End MU Setup:
*/
