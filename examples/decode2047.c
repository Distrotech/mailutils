/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2003 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <mailutils/mailutils.h>

#define ISPRINT(c) ((c)=='\t'||(c)==' '||(c)=='\n'||((c)>' '&&(c)<128)) 

void
print (char *p, int printable)
{
  for (; *p; p++)
    {
      if (printable && *p != '\n' && !ISPRINT (*p))
	printf ("\\%03o", *(unsigned char *) p);
      else
	putchar (*p);
    }
}

int
main (int argc, char *argv[])
{
  char buf[256];
  int c, printable = 0;

  while ((c = getopt (argc, argv, "p")) != EOF)
    switch (c)
      {
      case 'p':
	printable = 1;
	break;

      default:
	exit (1);
      }

  while (fgets (buf, sizeof (buf), stdin))
    {
      char *p = NULL;
      int rc, len;

      len = strlen (buf);
      if (len > 0 && buf[len - 1] == '\n')
	buf[len - 1] = 0;
      rc = rfc2047_decode ("iso-8859-1", buf, &p);
      printf ("%s=> %s\n", buf, mu_strerror (rc));
      if (p)
	print (p, printable);
      putchar ('\n');
      free (p);
    }      
    return 0;
}
