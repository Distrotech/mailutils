/* argcv.c - simple functions for parsing input based on whitespace
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

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

  *argv = calloc ((*argc + 1), sizeof (char *));

  for (i = 0; i <= len; i++)
    {
      if (command[i] == ' ' || command[i] == '\0')
	{
	  /* Reserve space for the null.  */
	  (*argv)[j] = calloc ((i - start + 1),  sizeof (char));
	  if ((*argv[j]) == NULL)
	    return 1;
	  strncpy ((*argv)[j], &command[start], i - start);
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
    if (argv[argc])
      free (argv[argc]);
  free (argv);
  return 1;
}

/* Take a argv an make string separated by ' '.  */

int
argcv_string (int argc, char **argv, char **pstring)
{
  int i;
  size_t len;
  char *buffer;

  /* No need.  */
  if (pstring == NULL)
    return 1;

  buffer = malloc (1);
  if (buffer == NULL)
    return 1;
  *buffer = '\0';

  for (len = i = 0; i < argc; i++)
    {
      len += strlen (argv[i] + 2);
      buffer = realloc (buffer, len);
      if (buffer == NULL)
	return 1;
      if (i != 0)
	strcat (buffer, " ");
      strcat (buffer, argv[i]);
    }

  /* Strip off trailing space.  */
  if (*buffer != '\0')
    {
      while (buffer[strlen (buffer) - 1] == ' ')
	{
	  buffer[strlen (buffer) - 1] = '\0';
	}
    }
  if (pstring)
    *pstring = buffer;
  return 0;
}
