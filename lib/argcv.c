/* argcv.c - simple functions for parsing input based on whitespace
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#include "argcv.h"

/*
 * takes a string and splits it into several strings, breaking at ' '
 * command is the string to split
 * the number of strings is placed into argc
 * the split strings are put into argv
 * returns 0 on success, nonzero on failure
 */

int
argcv_get (const char *command, int *argc, char ***argv)
{
  int len = strlen (command);
  int i = 0, j = 0;
  int start = 0;

  if (len < 1)
    return 1;

  *argc = 1;
  
  for (i = 0; i < len; i++)
    if (command[i] == ' ')
      (*argc)++;
  
  *argv = malloc ((*argc + 1) * sizeof (char *));

  for (i = 0; i <= len; i++)
    {
      if (command[i] == ' ' || command[i] == '\0')
	{
	  (*argv)[j] = malloc ((i-start) * sizeof (char));
	  if (argv[j] == NULL && (i-start > 0))
	    return 1;
	  strncpy ((*argv)[j], &command[start], i-start);
	  (*argv)[j][i-start] = '\0';
	  j++;
	  start = i+1;
	}
    }
  return 0;
}

/*
 * frees all elements of an argv array
 * argc is the number of elements
 * argv is the array
 */
int
argcv_free (int argc, char **argv)
{
  while (--argc >= 0)
    free (argv[argc]);
  free (argv);
  return 1;
}
