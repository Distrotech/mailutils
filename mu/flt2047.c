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

char flt2047_docstring[] = N_("decode/encode email message headers");
static char flt2047_args_doc[] = N_("[text]");

static int decode_mode = 0;
static int newline_option = 0;
static const char *charset = "iso-8859-1";
static const char *encoding = "quoted-printable";

static void
set_encode_mode (struct mu_parseopt *po, struct mu_option *opt,
		 char const *arg)
{
  decode_mode = 0;
}

static struct mu_option flt2047_options[] = {
  { "encode", 'e', NULL, MU_OPTION_DEFAULT,
    N_("encode the input (default)"),
    mu_c_string, NULL, set_encode_mode },
  { "decode", 'd', NULL, MU_OPTION_DEFAULT,
    N_("decode the input"),
    mu_c_bool, &decode_mode },
  { "newline", 'n', NULL, MU_OPTION_DEFAULT,
    N_("print additional newline"),
    mu_c_bool, &newline_option },  
  { "charset", 'c', N_("NAME"), MU_OPTION_DEFAULT,
    N_("set charset (default: iso-8859-1)"),
    mu_c_string, &charset },
  { "encoding", 'E', N_("NAME"), MU_OPTION_DEFAULT,
    N_("set encoding (default: quoted-printable)"),
    mu_c_string, &encoding },
  MU_OPTION_END
};
  
int
mutool_flt2047 (int argc, char **argv)
{
  int rc;
  char *p;

  mu_action_getopt (&argc, &argv, flt2047_options, flt2047_docstring,
		    flt2047_args_doc);

  if (argc)
    {
      char *p;

      while (argc--)
	{
	  const char *text = *argv++;
	  if (decode_mode)
	    rc = mu_rfc2047_decode (charset, text, &p);
	  else
	    rc = mu_rfc2047_encode (charset, encoding, text, &p);
	  if (rc)
	    {
	      mu_error ("%s", mu_strerror (rc));
	      return 1;
	    }
	  mu_printf ("%s\n", p);
	}
    }
  else
    {
      size_t size = 0, n;
      char *buf = NULL;

      while ((rc = mu_stream_getline (mu_strin, &buf, &size, &n)) == 0
	     && n > 0)
	{
	  mu_rtrim_class (buf, MU_CTYPE_SPACE);
	  if (decode_mode)
	    rc = mu_rfc2047_decode (charset, buf, &p);
	  else
	    rc = mu_rfc2047_encode (charset, encoding, buf, &p);
	  if (rc)
	    {
	      mu_error ("%s", mu_strerror (rc));
	      return 1;
	    }
	  mu_printf ("%s\n", p);
	}
    }
  mu_stream_flush (mu_strout);
  
  return 0;
}

/*
  MU Setup: 2047
  mu-handler: mutool_flt2047
  mu-docstring: flt2047_docstring
  End MU Setup:
*/
