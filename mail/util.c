/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

typedef struct _node {
  /* for the msglist expander */
  int data;
  /* for the environment table */
  struct mail_env_entry env_entry;
  struct _node *next;
} node;

static node *environment = NULL;
static node *env_cursor = NULL;

/*
 * add a new node to the list
 */
static node *
util_ll_add (node *c, int data)
{
  c->next = malloc (sizeof (node));
  c->data = data;
  c->next->env_entry.var = NULL;
  c->next->env_entry.set = 0;
  c->next->env_entry.value = NULL;
  c->next->next = NULL;
  return c->next;
}

/*
 * free a linked list
 */
static void
util_ll_free (node *c)
{
  node *t = c;
  while (t != NULL)
    {
      c = t;
      t = t->next;
      free (c);
    }
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
  int *ret = NULL;
  /* let's try a linked list */
  node *first = malloc (sizeof (node));
  node *current = first;
  first->next = NULL;

  if (util_command_get (argv[0]) == util_command_get ("undelete"))
    undelete = 1;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "+"))
	{
	  /* first [un]deleted message */
	  int n = realcursor + 1;
	  while (n <= total)
	    {
	      if (util_isdeleted (n) && !undelete)
		n++;
	      else
		break;
	    }
	  current = util_ll_add (current, n);
	}
      else if (!strcmp (argv[i], "-"))
	{
	  /* previous [un]deleted message */
	  int n = realcursor - 1;
	  while (n > 0)
	    {
	      if (util_isdeleted (n) && !undelete)
		n--;
	      else
		break;
	    }
	  current = util_ll_add (current, n);
	}
      else if (!strcmp (argv[i], "."))
	{
	  /* the current cursor location */
	  current = util_ll_add (current, realcursor);
	}
      else if (!strcmp (argv[i], "^"))
	{
	  /* first [un]deleted message */
	  int n = 1;
	  while (n <= total)
	    {
	      if (util_isdeleted (n) && !undelete)
		n++;
	      else
		break;
	    }
	  current = util_ll_add (current, 1);
	}
      else if (!strcmp (argv[i], "$"))
	{
	  /* last [un]deleted message */
	  int n = total;
	  while (n > 0)
	    {
	      if (util_isdeleted (n) && !undelete)
		n--;
	      else
		break;
	    }
	  current = util_ll_add (current, total);
	}
      else if (!strcmp (argv[i], "*"))
	{
	  /* all messages */
	  util_ll_free (first);
	  current = first = malloc (sizeof (node));
	  for (i = 1; i <= total; i++)
	    current = util_ll_add (current, i);
	  i = argc + 1;
	}
      else if (argv[i][0] == '/')
	{
	  /* FIXME: all messages with pattern following / in
	     the subject line, case insensitive */
	  /* This currently appears to be quit b0rked */
	  message_t msg;
	  header_t hdr;
	  char subj[128];
	  int j = 1, k = 0, l2 = 0;
	  int len = strlen (&argv[i][1]);
	  for (j = 1; j <= total; j++)
	    {
	      mailbox_get_message (mbox, j, &msg);
	      message_get_header (msg, &hdr);
	      header_get_value (hdr, MU_HEADER_SUBJECT, subj, 128, NULL);
	      l2 = strlen (subj);
	      for (k = 0; i < strlen (subj); k++)
		{
		  if (l2-k >= len && !strncasecmp (&argv[i][1], &subj[k], len))
		    {
		      current = util_ll_add (current, j);
		      k = 128;
		    }
		}
	    }
	}
      else if (argv[i][0] == ':')
	{
	  /* FIXME: all messages of type argv[i][1] */
	}
      else if (isalpha(argv[i][0]))
	{
	  /* FIXME: all messages from sender argv[i] */
	}
      else if (strchr (argv[i], '-') != NULL)
	{
	  /* message range */
	  int j, x, y;
	  char *arg = strdup (argv[i]);
	  for (j=0; j < strlen (arg); j++)
	    if (arg[j] == '-')
	      break;
	  arg[j] = '\0';
	  x = strtol (arg, NULL, 10);
	  y = strtol (&(arg[j + 1]), NULL, 10);
	  for (; x <= y; x++)
	    current = util_ll_add (current, x);
	  free (arg);
	}
      else
	{
	  /* single message */
	  current = util_ll_add (current, strtol (argv[i], NULL, 10));
	}
    }

  for (current = first; current != NULL; current = current->next)
    lc++;

  ret = malloc (lc * sizeof (int));
  lc = 0;
  for (current = first; current != NULL; current = current->next)
    ret [lc++] = current->data;
  util_ll_free (first);
  *list = ret;
  return lc-1;
}

/*
 * expands command into its command and arguments, then runs command
 * cmd is the command to parse and run
 * returns exit status of the command
 */
int
util_do_command (const char *c, ...)
{
  int argc = 0;
  char **argv = NULL;
  int status = 0;
  function_t *command;
  char *cmd = NULL;
  va_list ap;
  va_start (ap, c);
  if (vasprintf (&cmd, c, ap) < 1)
    return 0;

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
util_msglist_command (function_t *func, int argc, char **argv)
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
function_t *
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
 * removes whitespace from the beginning and end of a string
 */
char *
util_stripwhite (char *string)
{
  register char *s, *t;
  for (s = string; isspace ((unsigned)*s); s++)
    ;
  if (*s == 0)
    return s;
  t = s + strlen (s) - 1;
  while (t > s && isspace ((unsigned)*t))
    t--;
  *++t = '\0';
  return s;
}

/*
 * get the number of columns on the screen
 */
int
util_getcols (void)
{
    return strtol (getenv("COLUMNS"), NULL, 10);
}

/*
 * get the number of lines on the screen
 */
int
util_getlines (void)
{
    return strtol (getenv("LINES"), NULL, 10);
}

/*
 * find environment entry var
 */
struct mail_env_entry *
util_find_env (char *variable)
{
  char *var = variable;
  int len = strlen (var), need_free = 0;
  node *t;

  if (len < 1)
    return NULL;

  if (len == strlen ("ask") && !strcmp ("ask", var))
    {
      var = strdup ("asksub");
      len = strlen (var);
      need_free = 1;
    }

  if (environment == NULL)
    {
      environment = malloc (sizeof (node));
      environment->env_entry.var = NULL;
      environment->env_entry.set = 0;
      environment->env_entry.value = NULL;
      environment->next = NULL;
    }

  for (env_cursor = environment; env_cursor->next != NULL;
       env_cursor = env_cursor->next)
    {
      if (strlen (env_cursor->env_entry.var) == len &&
	  !strcmp (var, env_cursor->env_entry.var))
	{
	  if (need_free)
	    free (var);
	  return &(env_cursor->env_entry);
	}
    }

  env_cursor->env_entry.var = strdup (var);
  env_cursor->env_entry.set = 0;
  env_cursor->env_entry.value = NULL;
  t = env_cursor;
  env_cursor = util_ll_add (env_cursor, 0);
  if (need_free)
    free (var);
  return &(t->env_entry);
}

/*
 * print the environment
 */
int
util_printenv (int set)
{
  for (env_cursor = environment; env_cursor != NULL;
       env_cursor = env_cursor->next)
    {
      if (env_cursor->env_entry.set == set)
	{
	  printf ("%s", env_cursor->env_entry.var);
	  if (env_cursor->env_entry.value != NULL)
	    printf ("=%s", env_cursor->env_entry.value);
	  printf ("\n");
	}
    }
  return 0;
}

/*
 * return 1 if a message is deleted
 */
int
util_isdeleted (int n)
{
  message_t msg;
  attribute_t attr;
  if (mailbox_get_message (mbox, n, &msg) != 0)
    return 0;
  message_get_attribute (msg, &attr);
  if (attribute_is_deleted (attr))
    return 1;
  return 0;
}

/*
 * readline tab completion
 */
#ifdef WITH_READLINE
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

#else

char *
readline (const char *prompt)
{
  char *line;
  char *p;
  size_t linelen, total;

  if (prompt)
    {
      printf ("%s",prompt);
      fflush(stdout);
    }

  p = line = calloc (1, 255);
  linelen = 255;
  total = 0;
  for (;;)
    {
      size_t len;
      p = fgets (p, linelen, stdin);
      len = (p) ? strlen (p) : 0;

      total += len;

      /* Error.  */
      if (total == 0)
	{
	  free (line);
	  return NULL;
	}

      /* Ok.  */
      if (line[total - 1] == '\n')
	{
	  line[total - 1] = '\0';
	  return line;
	}
      else
        {
	  char *tmp;
	  linelen *= 2;
	  tmp = realloc (line, linelen);
	  if (tmp == NULL)
	    {
	      free (line);
	      return NULL;
	    }
	  line = tmp;
	  p = line + total;
	  total += len;
	}
    }
}
#endif
