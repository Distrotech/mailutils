/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2005, 2007, 2010 Free Software
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
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>

#define ISPRINT(c) ((c)>=' '&&(c)<127) 

int verbose = 0;
int printable = 0;
mu_stream_stat_buffer instat, outstat;

static void
c_copy (mu_stream_t out, mu_stream_t in)
{
  if (verbose)
    {
      mu_stream_set_stat (in,
			  MU_STREAM_STAT_MASK_ALL,
			  instat);
      mu_stream_set_stat (out,
			  MU_STREAM_STAT_MASK_ALL,
			  outstat);
    }
  
  if (printable)
    {
      char c;
      size_t size;
      
      while (mu_stream_read (in, &c, 1, &size) == 0 && size > 0)
	{
	  int rc;
	  
	  if (printable && !ISPRINT (c))
	    {
	      char outbuf[24];
	      sprintf (outbuf, "\\%03o", (unsigned char) c);
	      rc = mu_stream_write (out, outbuf, strlen (outbuf), NULL);
	    }
	  else
	    rc = mu_stream_write (out, &c, 1, NULL);
	}
    }
  else
    MU_ASSERT (mu_stream_copy (out, in, 0, NULL));

}

/* Set the maximum line length for the filter NAME to LENGTH.
   FIXME: This is a kludge. Perhaps API should provide a function
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

void
usage (const char *diag)
{
  FILE *fp;

  if (diag)
    {
      fp = stderr;
      fprintf (fp, "%s\n", diag);
    }
  else
    fp = stdout;

  fprintf (fp, "%s",
	   "usage: fltst FILTER {encode|decode} {read|write} [shift=N] [linelen=N] [verbose] [printable] [nl]\n");
  exit (diag ? 1 : 0);
}

int
main (int argc, char * argv [])
{
  mu_stream_t in, out, flt;
  int i;
  int mode = MU_FILTER_ENCODE;
  int flags = MU_STREAM_READ;
  char *fltname;
  mu_off_t shift = 0;
  size_t line_length;
  int line_length_option = 0;
  int newline_option = 0;
  
  if (argc == 1)
    usage (NULL);
  if (argc < 4)
    usage ("not enough arguments");
  
  fltname = argv[1];

  if (strcmp (argv[2], "encode") == 0)
    mode = MU_FILTER_ENCODE;
  else if (strcmp (argv[2], "decode") == 0)
    mode = MU_FILTER_DECODE;
  else
    usage ("2nd arg is wrong");

  if (strcmp (argv[3], "read") == 0)
    flags = MU_STREAM_READ;
  else if (strcmp (argv[3], "write") == 0)
    flags = MU_STREAM_WRITE;
  else
    usage ("3rd arg is wrong");

  for (i = 4; i < argc; i++)
    {
      if (strncmp (argv[i], "shift=", 6) == 0)
	shift = strtoul (argv[i] + 6, NULL, 0); 
      else if (strncmp (argv[i], "linelen=", 8) == 0)
	{
	  line_length = strtoul (argv[i] + 8, NULL, 10);
	  line_length_option = 1;
	}
      else if (strcmp (argv[i], "verbose") == 0)
	verbose++;
      else if (strcmp (argv[i], "printable") == 0)
	printable++;
      else if (strcmp (argv[i], "nl") == 0)
	newline_option++;
      else
	usage ("wrong option");
    }
  
  MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, 0));
  MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));

  if (line_length_option)
    reset_line_length (fltname, line_length);
  
  if (flags == MU_STREAM_READ)
    {
      MU_ASSERT (mu_filter_create (&flt, in, fltname, mode,
				   MU_STREAM_READ|MU_STREAM_SEEK|
				   MU_STREAM_AUTOCLOSE));
      if (shift)
	MU_ASSERT (mu_stream_seek (flt, shift, MU_SEEK_SET, NULL));
      c_copy (out, flt);
    }
  else
    {
      MU_ASSERT (mu_filter_create (&flt, out, fltname, mode,
				   MU_STREAM_WRITE));
      if (shift)
	MU_ASSERT (mu_stream_seek (in, shift, MU_SEEK_SET, NULL));
      c_copy (flt, in);

      mu_stream_close (in);
      mu_stream_destroy (&in);
    }

  mu_stream_close (flt);
  mu_stream_destroy (&flt);
  
  if (newline_option)
    mu_stream_write (out, "\n", 1, NULL);
    
  mu_stream_close (out);
  mu_stream_destroy (&out);
  
  if (verbose)
    {
      fprintf (stderr, "\nInput stream stats:\n");
      fprintf (stderr, "Bytes in: %lu\n",
	       (unsigned long) instat[MU_STREAM_STAT_IN]);
      fprintf (stderr, "Bytes out: %lu\n",
	       (unsigned long) instat[MU_STREAM_STAT_OUT]);
      fprintf (stderr, "Reads: %lu\n",
	       (unsigned long) instat[MU_STREAM_STAT_READS]);
      fprintf (stderr, "Seeks: %lu\n",
	       (unsigned long) instat[MU_STREAM_STAT_SEEKS]);

      fprintf (stderr, "\nOutput stream stats:\n");
      fprintf (stderr, "Bytes in: %lu\n",
	       (unsigned long) outstat[MU_STREAM_STAT_IN]);
      fprintf (stderr, "Bytes out: %lu\n",
	       (unsigned long) outstat[MU_STREAM_STAT_OUT]);
      fprintf (stderr, "Writes: %lu\n",
	       (unsigned long) outstat[MU_STREAM_STAT_WRITES]);
      fprintf (stderr, "Seeks: %lu\n",
	       (unsigned long) outstat[MU_STREAM_STAT_SEEKS]);
    }

  return 0;
}
