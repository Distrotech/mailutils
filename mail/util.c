/* GNU mailutils - a suite of utilities for electronic mail
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

#include "mail.h"

/*
 * takes a string and splits it into several strings, breaking at ' '
 * command is the string to split
 * the number of strings is placed into argc
 * the split strings are put into argv
 * returns 0 on success, nonzero on failure
 */

int
util_get_argcv (const char *command, int *argc, char ***argv)
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
  
  *argv = malloc (*argc * sizeof (char *));

  for (i = 0; i < len; i++)
    {
      if (command[i] == ' ')
	{
	  argv[j] = malloc ((i-start+1) * sizeof (char));
	  if (argv[j] == NULL)
	    return 1;
	  strncpy ((*argv)[j], &command[start], i-start);
	  j++;
	  start = i;
	}
    }
  return 0;
}

/*
 * expands a standard message list into an array of numbers
 * argc is the number of elements being passed in
 * argv is the array of strings to parse
 * list will be populated with message numbers
 * returns the number of items in list
 */

int
util_expand_msglist (const int argc, const char **argv, int **list)
{
  return 0;
}

/*
 * expands command into its command and arguments, then runs command
 * cmd is the command to parse and run
 * returns exit status of the command
 */
int
util_do_command (const char *cmd)
{
  int argc = 0;
  char **argv = NULL;
  int (*command) (int, char**) = NULL;

  if (cmd[0] == '#')
    return 0;

  if (util_get_argcv (cmd, &argc, &argv) != 0)
    return 1;

  /* FIXME: figure out which command to run */

  if (command != NULL)
    return command (argc, argv);

  return 1;
}

