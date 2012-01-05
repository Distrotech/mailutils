/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2002, 2007, 2010-2012 Free Software
   Foundation, Inc.

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

#include "mail.h"

static char *
source_readline (void *closure, int cont MU_ARG_UNUSED)
{
  mu_stream_t input = closure;
  size_t size = 0, n;
  char *buf = NULL;
  int rc;

  rc = mu_stream_getline (input, &buf, &size, &n);
  if (rc)
    {
      mu_diag_funcall (MU_DIAG_ERROR, "mu_stream_getline", NULL, rc);
      return NULL;
    }
  if (n == 0)
    return NULL;

  mu_rtrim_class (buf, MU_CTYPE_SPACE);
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
		   MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
  return buf;
}
  
/*
 * so[urce] file
 */

int
mail_source (int argc, char **argv)
{
  mu_stream_t input;
  int save_term;
  struct mu_locus locus;
  int rc;
  
  if (argc != 2)
    {
      /* TRANSLATORS: 'source' is a command name. Do not translate it! */
      mu_error (_("source requires a single argument"));
      return 1;
    }

  rc = mu_file_stream_create (&input, argv[1], MU_STREAM_READ);
  if (rc)
    {
      if (rc != ENOENT)
	mu_error(_("Cannot open `%s': %s"), argv[1], strerror (rc));
      return 1;
    }

  save_term = interactive;
  interactive = 0;
  locus.mu_file = argv[1];
  locus.mu_line = 0; 
  locus.mu_col = 0;
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS, &locus);
  mail_mainloop (source_readline, input, 0);
  interactive = save_term;
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS, NULL);
  mu_stream_unref (input);
  return 0;
}
