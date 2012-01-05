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
#include <mailutils/mailutils.h>
#include "argp.h"
#include "mu.h"

static char flt2047_doc[] = N_("mu 2047 - decode/encode message headers.");
char flt2047_docstring[] = N_("decode/encode email message headers");
static char flt2047_args_doc[] = N_("[text]");

static struct argp_option flt2047_options[] = {
  { "encode", 'e', NULL, 0, N_("encode the input (default)") },
  { "decode", 'd', NULL, 0, N_("decode the input") },
  { "newline", 'n', NULL, 0, N_("print additional newline") },
  { "charset", 'c', N_("NAME"), 0,
    N_("set charset (default: iso-8859-1)") },
  { "encoding", 'E', N_("NAME"), 0,
    N_("set encoding (default: quoted-printable)") },
  { NULL }
};
  
static int decode_mode = 0;
static int newline_option = 0;
static const char *charset = "iso-8859-1";
static const char *encoding = "quoted-printable";

static error_t
flt2047_parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'c':
      charset = arg;
      break;
      
    case 'e':
      decode_mode = 0;
      break;

    case 'E':
      encoding = arg;
      break;

    case 'd':
      decode_mode = 1;
      break;

    case 'n':
      newline_option = 0;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp flt2047_argp = {
  flt2047_options,
  flt2047_parse_opt,
  flt2047_args_doc,
  flt2047_doc,
  NULL,
  NULL,
  NULL
};

int
mutool_flt2047 (int argc, char **argv)
{
  int rc, index;
  char *p;
  
  if (argp_parse (&flt2047_argp, argc, argv, ARGP_IN_ORDER, &index, NULL))
    return 1;

  argc -= index;
  argv += index;

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
