/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003, 2007, 2010-2012 Free Software Foundation, Inc.

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

/* Functions shared between comp and forw utilities */

#include <mh.h>
#include <sys/types.h>
#include <sys/stat.h>

static char *default_format_str =
"To:\n"
"cc:\n"
"Subject:\n"
"--------\n";

void
mh_comp_draft (const char *formfile, const char *draftfile)
{
  char *s;

  if (mh_find_file (formfile, &s) == 0)
    {
      if (mh_file_copy (s, draftfile))
	exit (1);
      free (s);
    }
  else
    {
      /* I doubt if we need that: */
      int rc;
      mu_stream_t stream;
      
      if ((rc = mu_file_stream_create (&stream,
				       draftfile,
				       MU_STREAM_WRITE|MU_STREAM_CREAT)))
	{
	  mu_error (_("cannot open output file \"%s\": %s"),
		    draftfile, mu_strerror (rc));
	  exit (1);
	}
      
      rc = mu_stream_write (stream, 
			    default_format_str,
			    strlen (default_format_str), NULL);
      mu_stream_close (stream);
      mu_stream_destroy (&stream);
      
      if (rc)
	{
	  mu_error (_("error writing to \"%s\": %s"),
		    draftfile, mu_strerror (rc));
	  exit (1);
	}
    }
}

int 
check_draft_disposition (struct mh_whatnow_env *wh, int use_draft)
{
  struct stat st;
  int disp = DISP_REPLACE;

  /* First check if the draft exists */
  if (stat (wh->draftfile, &st) == 0)
    {
      if (use_draft)
	disp = DISP_USE;
      else
	{
	  printf (ngettext ("Draft \"%s\" exists (%s byte).\n",
                            "Draft \"%s\" exists (%s bytes).\n",
                            (unsigned long) st.st_size),
		  wh->draftfile, mu_umaxtostr (0, st.st_size));
	  disp = mh_disposition (wh->draftfile);
	}
    }

  return disp;
}
