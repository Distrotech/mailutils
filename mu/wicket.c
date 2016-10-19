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

#include "mu.h"

char wicket_docstring[] = N_("scan wickets for matching URLs");
static char wicket_args_doc[] = N_("URL");

static char *wicket_file = "~/.mu-tickets";
static int wicket_verbose = 1;

static void
clear_wicket_verbose (struct mu_parseopt *po, struct mu_option *opt,
		      char const *arg)
{
  wicket_verbose = 0;
}

static struct mu_option wicket_options[] = {
  { "file",    'f', N_("FILE"), MU_OPTION_DEFAULT,
    N_("use FILE instead of ~/.mu-tickets"),
    mu_c_string, &wicket_file },
  { "verbose", 'v', NULL,       MU_OPTION_DEFAULT,
    N_("increase output verbosity"),
    mu_c_string, &wicket_verbose },
  { "quiet",   'q', NULL,       MU_OPTION_DEFAULT,
    N_("suppress any output"),
    mu_c_string, NULL, clear_wicket_verbose },
  { NULL }
};


static int
wicket_match (mu_stream_t stream, const char *str)
{
  int rc, ret;
  mu_url_t u, url;
  struct mu_locus loc;
  int flags = MU_URL_PARSE_ALL;

  if (wicket_verbose > 2)
    flags &= ~MU_URL_PARSE_HIDEPASS;
  
  rc = mu_url_create (&u, str);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_url_create", str, rc);
      return 2;
    }

  rc = mu_stream_seek (stream, 0, MU_SEEK_SET, NULL);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_seek", "0", rc);
      return 2;
    }
  loc.mu_file = wicket_file;
  loc.mu_line = 0;
  rc = mu_wicket_stream_match_url (stream, &loc, u, flags, &url);
  switch (rc)
    {
    case 0:
      ret = 0;
      if (wicket_verbose)
	{
	  mu_printf ("%s: %s:%d", str, loc.mu_file, loc.mu_line);
	  if (wicket_verbose > 1)
	    mu_printf (": %s", mu_url_to_string (url));
	  mu_printf ("\n");
	}
      break;

    case MU_ERR_NOENT:
      if (wicket_verbose)
	mu_printf ("%s: %s\n", str, _("not found"));
      ret = 1;
      break;

    default:
      mu_diag_funcall (MU_DIAG_ERROR, "mu_wicket_stream_match_url", str, rc);
      ret = 2;
    }
  return ret;
}

int
mutool_wicket (int argc, char **argv)
{
  mu_stream_t stream;
  int rc, i, exit_code;
  
  mu_action_getopt (&argc, &argv, wicket_options, wicket_docstring,
		    wicket_args_doc);
  if (argc == 0)
    {
      mu_error (_("not enough arguments"));
      return 1;
    }

  wicket_file = mu_tilde_expansion (wicket_file, MU_HIERARCHY_DELIMITER, NULL);
  if (!wicket_file)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_tilde_expansion", wicket_file,
		       ENOMEM);
      return 2;
    }
  
  rc = mu_file_stream_create (&stream, wicket_file, MU_STREAM_READ);
  if (rc)
    {
      mu_error (_("cannot open input file %s: %s"), wicket_file,
		mu_strerror (rc));
      return 1;
    }

  exit_code = 0;
  for (i = 0; i < argc; i++)
    {
      rc = wicket_match (stream, argv[i]);
      if (rc)
	{
	  if (exit_code < rc)
	    exit_code = rc;
	  if (!wicket_verbose)
	    break;
	}
    }

  mu_stream_destroy (&stream);
  return exit_code;
}


/*
  MU Setup: wicket
  mu-handler: mutool_wicket
  mu-docstring: wicket_docstring
  End MU Setup:
*/
