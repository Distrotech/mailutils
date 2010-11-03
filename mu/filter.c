/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

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
#include "argp.h"
#include "mu.h"

static char filter_doc[] = N_("mu filter - apply a filter to the input");
char filter_docstring[] = N_("apply a filter to the input");
static char filter_args_doc[] = N_("NAME");

static struct argp_option filter_options[] = {
  { "encode", 'e', NULL, 0, N_("encode the input (default)") },
  { "decode", 'd', NULL, 0, N_("decode the input") },
  { "line-length", 'l', N_("NUMBER"), 0, N_("limit output line length") },
  { "newline", 'n', NULL, 0, N_("print additional newline") },
  { NULL }
};

static int filter_mode = MU_FILTER_ENCODE;
static int newline_option = 0;
static size_t line_length;
static int line_length_option = 0;

static error_t
filter_parse_opt (int key, char *arg, struct argp_state *state)
{
  char *p;
  
  switch (key)
    {
    case 'e':
      filter_mode = MU_FILTER_ENCODE;
      break;

    case 'd':
      filter_mode = MU_FILTER_DECODE;
      break;

    case 'n':
      newline_option = 1;
      break;

    case 'l':
      line_length = strtoul (arg, &p, 10);
      if (*p)
	argp_error (state, N_("not a number"));
      line_length_option = 1;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp filter_argp = {
  filter_options,
  filter_parse_opt,
  filter_args_doc,
  filter_doc,
  NULL,
  NULL,
  NULL
};

/* FIXME: This is definitely a kludge. The API should provide a function
   for that. */
static void
reset_line_length (const char *name, size_t length)
{
  mu_list_t list;
  int status;
  mu_filter_record_t frec;
  
  mu_filter_get_list (&list);
  status = mu_list_locate (list, (void*)name, (void**)&frec);
  if (status == 0)
    frec->max_line_length = length;
  /* don't bail out, leave that to mu_filter_create */
}

int
mutool_filter (int argc, char **argv)
{
  int rc, index;
  mu_stream_t in, out, flt;
  const char *fltname;
  
  if (argp_parse (&filter_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

  if (argc != 1)
    {
      mu_error (_("what filter do you want?"));
      return 1;
    }

  fltname = argv[0];
  
  rc = mu_stdio_stream_create (&in, MU_STDIN_FD, 0);
  if (rc)
    {
      mu_error (_("cannot open input stream: %s"), mu_strerror (rc));
      return 1;
    }
  
  rc = mu_stdio_stream_create (&out, MU_STDOUT_FD, 0);
  if (rc)
    {
      mu_error (_("cannot open output stream: %s"), mu_strerror (rc));
      return 1;
    }

  if (line_length_option)
    reset_line_length (fltname, line_length);

  rc = mu_filter_create (&flt, in, fltname, filter_mode, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("cannot open filter stream: %s"), mu_strerror (rc));
      return 1;
    }

  rc = mu_stream_copy (out, flt, 0, NULL);

  if (rc)
    {
      mu_error ("%s", mu_strerror (rc));
      return 1;
    }

  if (newline_option)
    mu_stream_write (out, "\n", 1, NULL);

  mu_stream_flush (out);
  
  return 0;
}

/*
  MU Setup: filter
  mu-handler: mutool_filter
  mu-docstring: filter_docstring
  End MU Setup:
*/
