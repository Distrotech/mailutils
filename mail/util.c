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
#include "table.h"

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
 * expands a standard message list into an array of numbers
 * argc is the number of elements being passed in
 * argv is the array of strings to parse
 * list will be populated with message numbers
 * returns the number of items in list
 */

int
util_expand_msglist (const int argc, char **argv, int **list)
{
  int i = 0, lc = 0;
  int undelete = 0;

  if (util_command_get (argv[0]) == util_command_get ("undelete"))
    undelete = 1;

  *list = malloc (argc * sizeof (int));
  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "+"))
	{
	  *list[lc++] = i; /* FIXME: next [un]deleted message */
	}
      else if (!strcmp (argv[i], "-"))
	{
	  *list[lc++] = i; /* FIXME: previous [un]deleted message */
	}
      else if (!strcmp (argv[i], "."))
	{
	  *list[lc++] = realcursor;
	}
      else if (!strcmp (argv[i], "^"))
	{
	  *list[lc++] = i; /* FIXME: first [un]deleted message */
	}
      else if (!strcmp (argv[i], "$"))
	{
	  *list[lc++] = i; /* FIXME: the last message */
	}
      else if (!strcmp (argv[i], "*"))
	{
	  free (*list);
	  *list = malloc (total * sizeof(int));
	  for (i=0; i < total; i++)
	    *list[i] = i+1;
	  return total;
	}
      else if (argv[i][0] == '/')
	{
	  *list[lc++] = i; /* FIXME: all messages with pattern following / in
			     the subject line, case insensitive */
	}
      else if (argv[i][0] == ':')
	{
	  *list[lc++] = i; /* FIXME: all messages of type argv[i][1] */
	}
      else if (isalpha(argv[i][0]))
	{
	  *list[lc++] = i; /* FIXME: all messages from argv[i] */
	}
      else
	{
	  int j = 0, hyphen = 0;
	  int len = strlen (argv[i]);
	  
	  for (j=0; j < len; j++)
	    if (argv[i][j] == '-')
	      hyphen = 1;

	  if (!hyphen)
	    *list[lc++] = atoi (argv[i]);
	  else
	    {
	      *list[lc++] = i; /* FIXME: range in argv[i] (x-y) */
	    }
	}
    }
  return lc;
}

/*
 * frees all elements of an argv array
 * argc is the number of elements
 * argv is the array
 */
int
util_free_argv (int argc, char **argv)
{
  while (--argc >= 0)
    free (argv[argc]);
  free (argv);
  return 1;
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
  int status = 0;
  int (*command) (int, char**) = NULL;

  if (cmd[0] == '#')
    return 0;

  if (util_get_argcv (cmd, &argc, &argv) != 0)
    return util_free_argv (argc, argv);

  command = util_command_get (argv[0]);

  if (command != NULL)
    status = command (argc, argv);
  else
    {
      printf ("Unknown command: %s\n", argv[0]);
      status = 1;
    }

  util_free_argv (argc, argv);
  return status;
}

/*
 * runs a function repeatedly on a msglist
 * func is the function to run
 * argc is the number of arguments inculding the command and msglist
 * argv is the list of strings containing the command and msglist
 */

int
util_msglist_command (int (*func)(int, char**), int argc, char **argv)
{
  int i;
  int *list;
  int status = 0;
  int number = util_expand_msglist (argc, argv, &list);
  realcursor = cursor;

  for (i = 0; i <= number; i++)
    {
      cursor = list[i];
      if (func (1, argv) != 0)
	status = 1;
    }

  cursor = realcursor;
  return status;
}

/*
 * returns the function to run for command
 */
int *
util_command_get (char *cmd)
{
  int i = 0;
  int len = strlen (cmd);
  int sl, ll;
  
  while (mail_command_table[i].shortname != 0)
    {
      sl = strlen (mail_command_table[i].shortname);
      ll = strlen (mail_command_table[i].longname);
      if (sl == len && !strcmp (mail_command_table[i].shortname, cmd))
	return mail_command_table[i].func;
      else if (sl < len && !strncmp (mail_command_table[i].longname, cmd, len))
	return mail_command_table[i].func;
      i++;
    }

  return NULL;
}
