/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <mailutils/mailutils.h>

int
main (int argc, char * argv [])
{
  stream_t in, out, flt;
  unsigned char buffer;
  int c, verbose = 0;
  int printable = 0;
  size_t size, total = 0;
  int mode = MU_FILTER_ENCODE;
  char *input = NULL, *output = NULL;

  while ((c = getopt (argc, argv, "dehi:o:pv")) != EOF)
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
	
      case 'e':
	mode = MU_FILTER_ENCODE;
	break;

      case 'p':
	printable = 1;
	break;
	
      case 'v':
	verbose = 1;
	break;

      case 'h':
	printf ("usage: base64 [-vpde][-i infile][-o outfile]\n");
	exit (0);
	  
      default:
	exit (1);
      }

  if (input)
    c = file_stream_create (&in, input, MU_STREAM_READ);
  else
    c = stdio_stream_create (&in, stdin, 0);
  assert (c == 0);
  assert (filter_create (&flt, in, "base64", mode, MU_STREAM_READ) == 0);
  assert (stream_open (in) == 0);

  if (output)
    c = file_stream_create (&out, output, MU_STREAM_WRITE|MU_STREAM_CREAT);
  else
    c = stdio_stream_create (&out, stdout, 0);
  assert (c == 0);
  assert (stream_open (out) == 0);
  
  while (stream_read (flt, &buffer, sizeof (buffer), total, &size) == 0
	 && size > 0)
    {
      if (printable && !isprint (buffer))
	{
	  char outbuf[24];
	  sprintf (outbuf, "\\%03o", (unsigned int) buffer);
	  stream_write (out, outbuf, strlen (outbuf), total, NULL);
	}
      else
	stream_write (out, &buffer, size, total, NULL);
      total += size;
    }

  stream_write (out, "\n", 1, total, NULL);
  stream_close (in);
  stream_close (out);
  if (verbose)
    fprintf (stderr, "total: %lu bytes\n", (unsigned long) total);
  return 0;
}
