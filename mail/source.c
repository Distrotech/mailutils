/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#include "mail.h"

/*
 * so[urce] file
 */

int
mail_source (int argc, char **argv)
{
#if 1

  /* On sparky's machine, there is an odd SEGV coming from a free() deep
     withing fopen(). I don't get it */

  if (argc == 2)
    {
      FILE *rc = fopen (argv[1], "r");
      if (rc != NULL)
	{
	  char *buf = NULL;
	  size_t s = 0;
	  while (getline (&buf, &s, rc) >= 0)
	    {
	      buf[strlen(buf) - 1] = '\0';
	      util_do_command("%s", buf);
	      free (buf);
	      buf = NULL;
	      s = 0;
	    }
	  fclose (rc);
	  return 0;
	}
    }
#endif
  return 1;
}
