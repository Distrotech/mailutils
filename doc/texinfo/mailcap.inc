@comment   GNU Mailutils -- a suite of utilities for electronic mail
@comment   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.
@comment
@comment   GNU Mailutils is free software; you can redistribute it and/or modify
@comment   it under the terms of the GNU General Public License as published by
@comment   the Free Software Foundation; either version 2, or (at your option)
@comment   any later version.
@comment
@comment   GNU Mailutils is distributed in the hope that it will be useful,
@comment   but WITHOUT ANY WARRANTY; without even the implied warranty of
@comment   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@comment   GNU General Public License for more details.
@comment
@comment   You should have received a copy of the GNU General Public License
@comment   along with GNU Mailutils; if not, write to the Free Software
@comment   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  

#include <stdio.h>
#include <mailutils/mailcap.h>
#include <mailutils/stream.h>
#include <mailutils/error.h>

int
main(int argc, char **argv)
@{
  stream_t stream = NULL;
  int status = 0;
  char *file = argc == 1 ? "/etc/mailcap" : argv[1];
  mu_mailcap_t mailcap = NULL;
    
  status = file_stream_create (&stream, file, MU_STREAM_READ);
  if (status)
    @{
      mu_error ("cannot create file stream %s: %s",
		file, mu_strerror (status));
      exit (1);
    @}

  status = stream_open (stream);
  if (status)
    @{
      mu_error ("cannot open file stream %s: %s",
		file, mu_strerror (status));
      exit (1);
    @}

  status = mu_mailcap_create (&mailcap, stream);
  if (status == 0)
    @{
      int i;
      size_t count = 0;
      char buffer[256];

      mu_mailcap_entries_count (mailcap, &count);
      for (i = 1; i <= count; i++)
	@{
	  int j;
	  mu_mailcap_entry_t entry = NULL;
	  int fields_count = 0;

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
	    @{
	      mu_mailcap_entry_get_field (entry, j, buffer, 
					  sizeof (buffer), NULL);
	      printf ("\tfields[%d]: %s\n", j, buffer);
	    @}
	  printf ("\n");
	@}
      mu_mailcap_destroy (&mailcap);
    @}
  
  return 0;
@}
