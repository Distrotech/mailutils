/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2004, 2007, 2009-2012 Free Software
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <mailutils/mailcap.h>
#include <mailutils/stream.h>
#include <mailutils/error.h>

int
main (int argc, char **argv)
{
  mu_stream_t stream = NULL;
  int status = 0;
  mu_mailcap_t mailcap = NULL;

  status = mu_stdio_stream_create (&stream, MU_STDIN_FD,
				   MU_STREAM_READ|MU_STREAM_SEEK);
  if (status)
    {
      mu_error ("cannot create input stream: %s",
		mu_strerror (status));
      exit (1);
    }

  status = mu_mailcap_create (&mailcap, stream);
  if (status == 0)
    {
      int i;
      size_t count = 0;
      char buffer[256];

      mu_mailcap_entries_count (mailcap, &count);
      for (i = 1; i <= count; i++)
	{
	  size_t j;
	  mu_mailcap_entry_t entry = NULL;
	  size_t fields_count = 0;

	  printf ("entry[%d]\n", i);

	  mu_mailcap_get_entry (mailcap, i, &entry);

	  /* typefield.  */
	  mu_mailcap_entry_get_typefield (entry, buffer, 
					  sizeof (buffer), NULL);
	  printf ("\ttypefield: %s\n", buffer);
	  
	  /* view-command.  */
	  mu_mailcap_entry_get_viewcommand (entry, buffer, 
					    sizeof (buffer), NULL);
	  printf ("\tview-command: %s\n", buffer);

	  /* fields.  */
	  mu_mailcap_entry_fields_count (entry, &fields_count);
	  for (j = 1; j <= fields_count; j++)
	    {
	      int status = mu_mailcap_entry_get_field (entry, j, buffer, 
						       sizeof (buffer), NULL);
	      if (status)
		{
		  mu_error ("cannot retrieve field %lu: %s",
			    (unsigned long) j,
			    mu_strerror (status));
		  break;
		}
	      printf ("\tfields[%lu]: %s\n", (unsigned long) j, buffer);
	    }
	  printf ("\n");
	}
      mu_mailcap_destroy (&mailcap);
    }
  
  return 0;
}
