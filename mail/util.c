/* GNU mailutils - a suite of utilities for electronic mail
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

#include "mail.h"

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
  int hyphen = 0;

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
	  hyphen = 1; /* FIXME: previous [un]deleted message */
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
	  for (i = 0; i < total; i++)
	    *list[i] = i + 1;
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
	  int j;
	  int len = strlen (argv[i]);

	  for (j = 0; j < len; j++)
	    if (argv[i][j] == '-')
	      {
		hyphen = 1;
		break;
	      }
	  if (hyphen)
            {
              if (j != len) /* One argument "x-y".  */
                {
                  int x, y;
                  char *arg = strdup (argv[i]);
                  arg[j] = '\0';
                  x = atoi (arg);
                  y = atoi (&(arg[j + 1]));
                  /* In this case, we also have to realloc() the list.  */
                  *list = realloc (*list, (argc + 2) * sizeof (int));
                  for (; x <= y; x++, lc++)
                    *list[lc] = x;
                  free (arg);
                }
              else if (i == 3)  /* 3 arguments "x" "-" "y".  */
                {
                  int x, y;
                  x = *list[lc - 1];
                  y = atoi (argv[i]);
                  for (; x <= y; x++, lc++)
                    *list[lc] = x;
                }
              else  /* Badly form.  */
                *list[lc++] = atoi (argv[i]);
              hyphen = 0;
            }
          else
            *list[lc++] = atoi(argv[i]);
	}
    }
  return lc;
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
  Function *command;

  if (cmd[0] == '#')
    return 0;

  if (cmd)
    {
      if (argcv_get (cmd, &argc, &argv) != 0)
	return argcv_free (argc, argv);
      command = util_command_get (argv[0]);
    }
  else
    command = util_command_get ("quit");

  if (command != NULL)
    status = command (argc, argv);
  else
    {
      printf ("Unknown command: %s\n", argv[0]);
      status = 1;
    }

  argcv_free (argc, argv);
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
  int *list = NULL;
  int status = 0;
  int number = util_expand_msglist (argc, argv, &list);
  realcursor = cursor;

  for (i = 0; i < number; i++)
    {
      cursor = list[i];
      if (func (1, argv) != 0)
	status = 1;
    }
  free (list);

  cursor = realcursor;
  return status;
}

/*
 * returns the function to run for command
 */
Function *
util_command_get (char *cmd)
{
  struct mail_command_entry entry = util_find_entry (cmd);
  return entry.func;
}

/*
 * returns the mail_command_entry structure for the command matching cmd
 */
struct mail_command_entry
util_find_entry (char *cmd)
{
  int i = 0, ll = 0, sl = 0;
  int len = strlen (cmd);

  while (mail_command_table[i].shortname != 0)
    {
      sl = strlen (mail_command_table[i].shortname);
      ll = strlen (mail_command_table[i].longname);
      if (sl > ll && !strncmp (mail_command_table[i].shortname, cmd, sl))
	return mail_command_table[i];
      else if (sl == len && !strcmp (mail_command_table[i].shortname, cmd))
	return mail_command_table[i];
      else if (sl < len && !strncmp (mail_command_table[i].longname, cmd, len))
	return mail_command_table[i];
      i++;
    }
  return mail_command_table[i];
}

/*
 * readline tab completion
 */
char **
util_command_completion (char *cmd, int start, int end)
{
  if (start == 0)
    return completion_matches (cmd, util_command_generator);
  return NULL;
}

/*
 * more readline
 */
char *
util_command_generator (char *text, int state)
{
  static int i, len;
  char *name;

  if (!state)
    {
      i = 0;
      len = strlen (text);
    }

  while ((name = mail_command_table[i].longname))
    {
      if (strlen (mail_command_table[i].shortname) > strlen(name))
	name = mail_command_table[i].shortname;
      i++;
      if (strncmp (name, text, len) == 0)
	return (strdup(name));
    }

  return NULL;
}

/*
 * removes whitespace from the beginning and end of a string
 */
char *
util_stripwhite (char *string)
{
  register char *s, *t;
  for (s = string; whitespace (*s); s++)
    ;
  if (*s == 0)
    return s;
  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';
  return s;
}


