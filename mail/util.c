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
#include <mailutils/mutil.h>
#include <pwd.h>

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

  for (current = first; current->next != NULL; current = current->next)
    lc++;

  if (!lc)
    {
      ret = calloc (1, sizeof (int));
      if (!ret)
	{
	  fprintf (ofile, "not enough memory\n");
	  exit (1);
	}
      ret [0] = cursor;
      lc = 1;
    }
  else
    {
      ret = malloc (lc * sizeof (int));
      if (!ret)
	{
	  fprintf (ofile, "not enough memory\n");
	  exit (1);
	}
      lc = 0;
      for (current = first; current->next != NULL; current = current->next)
	ret [lc++] = current->data;
    }
  util_ll_free (first);
  *list = ret;
  return lc;
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
  int i, zcnt = 0;
  
  va_start (ap, c);
  status = vasprintf (&cmd, c, ap);
  va_end (ap);
  if (status < 0)
    return 0;
  
  if (cmd)
    {
      struct mail_command_entry entry;

      if (cmd[0] == '#')
	return 0;

      if (argcv_get (cmd, &argc, &argv) != 0)
	return argcv_free (argc, argv);

      /* Eliminate empty strings */
      for (i = 0; i < argc; i++)
	{
	  if (argv[i][0] == 0)
	    {
	      int d;
	      for (d = i; d < argc && argv[d][0] == 0; d++)
		;
	      if (d == argc)
		{
		  break;
		}
	      else
		{
		  char *s = argv[d];
		  argv[d] = argv[i];
		  argv[i] = s;
		}
	      zcnt++;
	    }
	}
      
      entry = util_find_entry (argv[0]);

      if (if_cond() == 0 && entry.isflow == 0)
	{
	  argcv_free (argc, argv);
	  return 0;
	}
      command = entry.func;
    }
  else
    command = util_command_get ("quit");

  if (command != NULL)
    status = command (argc - zcnt, argv);
  else
    {
      fprintf (ofile, "Unknown command: %s\n", argv[0]);
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
 * set_cursor means whether the function should set the cursor to 
 * the number of the last message processed. If set_cursor = 0, the
 * cursor is not altered.
 */

int
util_msglist_command (function_t *func, int argc, char **argv, int set_cursor)
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

  if (set_cursor)
    realcursor = cursor;
  else
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

int
util_screen_lines()
{
  struct mail_env_entry *ep = util_find_env("screen");
  size_t n;
  
  if (ep && ep->set && (n = atoi(ep->value)) != 0)
    return n;
  return util_getlines();
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
	  fprintf (ofile, "%s", env_cursor->env_entry.var);
	  if (env_cursor->env_entry.value != NULL)
	    fprintf (ofile, "=%s", env_cursor->env_entry.value);
	  fprintf (ofile, "\n");
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
  size_t alloclen, linelen;

  if (prompt)
    {
      fprintf (ofile, "%s",prompt);
      fflush (ofile);
    }

  p = line = calloc (1, 255);
  alloclen = 255;
  linelen = 0;
  for (;;)
    {
      size_t n;
      p = fgets (p, alloclen - linelen, stdin);
      n = (p) ? strlen (p) : 0;

      linelen += n;

      /* Error.  */
      if (linelen == 0)
	{
	  free (line);
	  return NULL;
	}

      /* Ok.  */
      if (line[linelen - 1] == '\n')
	{
	  line[linelen - 1] = '\0';
	  return line;
	}
      else
        {
	  char *tmp;
	  alloclen *= 2;
	  tmp = realloc (line, alloclen);
	  if (tmp == NULL)
	    {
	      free (line);
	      return NULL;
	    }
	  line = tmp;
	  p = line + linelen;
	}
    }
}
#endif

char *
util_get_homedir()
{
  char *homedir = mu_get_homedir();
  if (!homedir)
    {
      /* Shouldn't happen, but one never knows */
      fprintf(ofile, "can't get homedir\n");
      exit (EXIT_FAILURE);
    }
  return strdup(homedir);
}
		 
char *
util_fullpath(char *inpath)
{
  return mu_tilde_expansion(inpath, "/", NULL);
}

char *
util_get_sender(int msgno, int strip)
{
  header_t header = NULL;
  address_t addr = NULL;
  message_t msg = NULL;
  char buffer[512], *p;
      
  mailbox_get_message (mbox, msgno, &msg);
  message_get_header (msg, &header);
  if (header_get_value (header, MU_HEADER_FROM, buffer, sizeof(buffer), NULL)
      || address_create (&addr, buffer))
    {
      envelope_t env = NULL;
      message_get_envelope (msg, &env);
      if (envelope_sender (env, buffer, sizeof (buffer), NULL)
	  || address_create (&addr, buffer))
	{
	  fprintf (ofile, "can't determine sender name (msg %d)\n", msgno);
	  return NULL;
	}
    }

  if (address_get_email (addr, 1, buffer, sizeof(buffer), NULL))
    {
      fprintf (ofile, "can't determine sender name (msg %d)\n", msgno);
      address_destroy (&addr);
      return NULL;
    }
  
  if (strip)
    {
      p = strchr (buffer, '@');
      if (p)
	*p = 0;
    }
  
  p = strdup (buffer);
  address_destroy (&addr);
  return p;
}

void
util_slist_print(list_t list, int nl)
{
  iterator_t itr;
  char *name;
  
  if (!list || iterator_create (&itr, list))
    return;
  
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      fprintf (ofile, "%s%c", name, nl ? '\n' : ' ');
      
    }
  iterator_destroy (&itr);
}

int
util_slist_lookup(list_t list, char *str)
{
  iterator_t itr;
  char *name;
  int rc = 0;
  
  if (!list || iterator_create (&itr, list))
    return 0;
  
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      if (strcasecmp (name, str) == 0)
	{
	  rc = 1;
	  break;
	}
    }
  iterator_destroy (&itr);
  return rc;
}

void
util_slist_add (list_t *list, char *value)
{
  char *p;
  
  if (!*list && list_create (list))
    return;
  
  if ((p = strdup(value)) == NULL)
    {
      fprintf (ofile, "not enough memory\n");
      return;
    }
  list_append (*list, p);
}

void
util_slist_destroy (list_t *list)
{
  iterator_t itr;
  char *name;
  
  if (!*list || iterator_create (&itr, *list))
    return;
  
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      free (name);
    }
  iterator_destroy (&itr);
  list_destroy (list);
}

char *
util_slist_to_string (list_t list, char *delim)
{
  iterator_t itr;
  char *name;
  char *str = NULL;
  
  if (!list || iterator_create (&itr, list))
    return NULL;
  
  for (iterator_first (itr); !iterator_is_done (itr); iterator_next (itr))
    {
      iterator_current (itr, (void **)&name);
      if (str && delim)
	util_strcat(&str, delim);
      util_strcat(&str, name);
    }
  iterator_destroy (&itr);
  return str;
}

void
util_strcat(char **dest, char *str)
{
  if (!*dest)
    *dest = strdup (str);
  else
    {
      int dlen = strlen (*dest) + 1;
      int slen = strlen (str) + 1;
      char *newp = realloc (*dest, dlen + slen);

      if (!newp)
	return;

      *dest = newp;
      memcpy (newp + dlen - 1, str, slen);
    }
}
      
void
util_escape_percent (char **str)
{
  int count;
  char *p, *q;
  char *newstr;
  
  /* Count ocurrences of % in the string */
  count = 0;
  for (p = *str; *p; p++)
    if (*p == '%')
      count++;

  if (!count)
    return; /* nothing to do */

  /* expand the string */ 
  newstr = malloc (strlen (*str) + 1 + count);
  if (!newstr)
    {
      fprintf (ofile, "not enough memory\n");
      exit (1); /* be on the safe side */
    }

  /* and escape percent signs */ 
  p = newstr;
  q = *str;
  while (*p = *q++)
    {
      if (*p == '%')
	*++p = '%';
      p++;
    }
  *str = newstr;
}

char *
util_outfolder_name (char *str)
{
  struct mail_env_entry *ep = util_find_env("outfolder");

  switch (*str)
    {
    case '/':
    case '~':
    case '+':
      str = util_fullpath (str);
      break;
      
    default:
      if (ep && ep->set)
	{
	  char *ns = NULL;
	  asprintf (&ns, "%s/%s", ep->value, str);
	  str = util_fullpath (ns);
	  free (ns);
	}
      break;

    }

  return str;
}

char *
util_whoami()
{
  struct passwd *pw = getpwuid(getuid());
  return pw ? pw->pw_name : "unknown";
}

/* Save an outgoing message. "savefile" allows to override the setting
   of the "record" variable. */
void
util_save_outgoing (message_t msg, char *savefile)
{
  struct mail_env_entry *ep = util_find_env("record");
  if (ep->set)
    {
      FILE *outfile;
      char *filename = util_outfolder_name (savefile ? savefile : ep->value);

      outfile = fopen (filename, "a");
      if (!outfile)
	{
	  fprintf (outfile, "can't open save file %s: %s",
		   filename, strerror (errno));
	}
      else
	{
	  char *buf;
	  size_t bsize = 0;
	  
	  message_size (msg, &bsize);

	  /* Try to allocate large buffer */
	  for (; bsize > 1; bsize /= 2)
	    if ((buf = malloc (bsize)))
	      break;
	  
	  if (!bsize)
	    {
	      fprintf (ofile, "not enough memory for creating save file\n");
	    }
	  else
	    {
	      stream_t stream;
	      size_t n, off = 0;
	      time_t t;
	      struct tm *tm;
	      char date[64];

	      time(&t);
	      tm = gmtime(&t);
	      strftime (date, sizeof (date), "%a %b %e %H:%M:%S %Y%n", tm);
	      fprintf (outfile, "From %s %s\n", util_whoami(), date);

	      message_get_stream (msg, &stream);
	      while (stream_read (stream, buf, bsize, off, &n) == 0
		     && n != 0)
		{
		  fwrite (buf, 1, n, outfile);
		  off += n;
		}
	      free (buf);
	    }
	  fclose (outfile);
	}
      free (filename);
    }
}
  




