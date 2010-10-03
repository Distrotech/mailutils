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
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

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

static void
c_copy (mu_stream_t out, mu_stream_t in)
{
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
  mu_stream_write (out, "\n", 1, NULL);
  mu_stream_close (out);
  mu_stream_close (in);
  if (verbose)
    fprintf (stderr, "\ntotal: %lu/%lu bytes\n",
	     (unsigned long) mu_stream_bytes_in (in),
	     (unsigned long) mu_stream_bytes_out (out));
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

int
main (int argc, char * argv [])
{
  mu_stream_t in, out, flt;
  int c;
  int mode = MU_FILTER_ENCODE;
  int flags = MU_STREAM_READ;
  char *input = NULL, *output = NULL;
  char *encoding = "base64";
  mu_off_t shift = 0;
  size_t line_length;
  int line_length_option = 0;
  
  while ((c = getopt (argc, argv, "deE:hi:l:o:ps:vw")) != EOF)
    switch (c)
      {
      case 'i':
	input = optarg;
	break;

      case 'o':
	output = optarg;
	break;
	
      case 'd':
	mode = MU_FILTER_DECODE;
	break;

      case 'E':
	encoding = optarg;
	break;
	
      case 'e':
	mode = MU_FILTER_ENCODE;
	break;

      case 'l':
	line_length = strtoul (optarg, NULL, 10);
	line_length_option = 1;
	break;
	
      case 'p':
 	printable = 1;
	break;

      case 's':
	shift = strtoul (optarg, NULL, 0); 
	break;
	
      case 'v':
	verbose = 1;
	break;

      case 'h':
	printf ("usage: base64 [-vpde][-E encoding][-i infile][-o outfile]\n");
	exit (0);

      case 'w':
	flags = MU_STREAM_WRITE;
	break;
	
      default:
	exit (1);
      }

  if (input)
    MU_ASSERT (mu_file_stream_create (&in, input, MU_STREAM_READ|MU_STREAM_SEEK));
  else
    MU_ASSERT (mu_stdio_stream_create (&in, MU_STDIN_FD, 0));

  if (output)
    MU_ASSERT (mu_file_stream_create (&out, output, 
                                      MU_STREAM_WRITE|MU_STREAM_CREAT));
  else
    MU_ASSERT (mu_stdio_stream_create (&out, MU_STDOUT_FD, 0));

  if (line_length_option)
    reset_line_length (encoding, line_length);
  
  if (flags == MU_STREAM_READ)
    {
      MU_ASSERT (mu_filter_create (&flt, in, encoding, mode,
				   MU_STREAM_READ|MU_STREAM_SEEK|
				   MU_STREAM_AUTOCLOSE));
      if (shift)
	MU_ASSERT (mu_stream_seek (flt, shift, MU_SEEK_SET, NULL));
      c_copy (out, flt);
    }
  else
    {
      MU_ASSERT (mu_filter_create (&flt, out, encoding, mode,
				   MU_STREAM_WRITE|MU_STREAM_AUTOCLOSE));
      if (shift)
	MU_ASSERT (mu_stream_seek (in, shift, MU_SEEK_SET, NULL));
      c_copy (flt, in);
    }
      
  return 0;
}
