/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2007, 2010 Free Software Foundation,
   Inc.

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
  FILE *fp = closure;
  size_t s = 0;
  char *buf = NULL;
  
  if (getline (&buf, &s, fp) >= 0)
    {
      mu_rtrim_class (buf, MU_CTYPE_SPACE);
      mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                       MU_IOCTL_LOGSTREAM_ADVANCE_LOCUS_LINE, NULL);
      return buf;
    }
  
  return NULL;
}
  
/*
 * so[urce] file
 */

int
mail_source (int argc, char **argv)
{
  FILE *fp;
  int save_term;
  struct mu_locus locus;
  
  if (argc != 2)
    {
      /* TRANSLATORS: 'source' is a command name. Do not translate it! */
      util_error (_("source requires a single argument"));
      return 1;
    }
  
  fp = fopen (argv[1], "r");
  if (!fp)
    {
      if (errno != ENOENT)
	util_error(_("Cannot open `%s': %s"), argv[1], strerror(errno));
      return 1;
    }

  save_term = interactive;
  interactive = 0;
  locus.mu_file = argv[1];
  locus.mu_line = 0; 
  locus.mu_col = 0;
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS, &locus);
  mail_mainloop (source_readline, fp, 0);
  interactive = save_term;
  mu_stream_ioctl (mu_strerr, MU_IOCTL_LOGSTREAM,
                   MU_IOCTL_LOGSTREAM_SET_LOCUS, NULL);
  fclose (fp);
  return 0;
}
