/* nntpclient.c -- An application which demonstrates how to use the
   GNU Mailutils nntp functions.  This application interactively allows users
   to contact a nntp server.

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <termios.h>
#include <signal.h>
#include <ctype.h>

#ifdef WITH_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

#include <mailutils/nntp.h>
#include <mailutils/iterator.h>
#include <mailutils/error.h>
#include <mailutils/errno.h>

/* A structure which contains information on the commands this program
   can understand. */

typedef struct
{
  const char *name;		/* User printable name of the function. */
  int (*func) (char *);		/* Function to call to do the job. */
  const char *doc;		/* Documentation for this function.  */
}
COMMAND;

/* The names of functions that actually do the manipulation. */
int com_article (char *);
int com_body (char *);
int com_date (char *);
int com_group (char *);
int com_head (char *);
int com_last (char *);
int com_list_extensions (char *);
int com_mode_reader (char *);
int com_next (char *);
int com_quit (char *);
int com_mode_reader (char *);

int com_exit (char *);

int com_help (char *);
int com_stat (char *);

int com_connect (char *);
int com_disconnect (char *);
int com_verbose (char *);

void initialize_readline (void);
char *stripwhite (char *);
COMMAND *find_command (char *);
char *dupstr (const char *);
int execute_line (char *);
int valid_argument (const char *, char *);

void sig_int (int);

COMMAND commands[] = {
  {"mode reader", com_mode_reader, "Set mode reader: MODE READER"},
  {"list extensions", com_list_extensions, "List extensions: LIST EXTENSIONS"},
  {"quit", com_quit, "Terminate the session: QUIT"},
  {"group", com_group, "Select a group: GROUP group"},
  {"next", com_next, "Set current to the next article: NEXT"},
  {"last", com_last, "Set current to the previous article: LAST"},

  {"article", com_article, "Retrieve an article: ARTICLE [message_id|number]"},
  {"header", com_head, "Retrieve the head of an article: HEAD [message_id|number]"},
  {"body", com_body, "Retrieve the body of an article: BODY [message_id|number]"},

  {"date", com_date, "Server date: DATE"},
  {"stat", com_stat, "Check the status of an article : STAT [message_id|number]"},

  {"connect", com_connect, "Open connection: connect hostname [port]"},
  {"disconnect", com_disconnect, "Close connection: disconnect"},

  {"verbose", com_verbose, "Enable Protocol tracing: verbose [on|off]"},
  {"exit", com_exit, "exit program"},

  {"help", com_help, "Display this text"},
  {"?", com_help, "Synonym for `help'"},
  {NULL, NULL, NULL}
};

/* The name of this program, as taken from argv[0]. */
char *progname;

/* Global handle for nntp.  */
mu_nntp_t nntp;

/* Flag if verbosity is needed.  */
int verbose;

/* When non-zero, this global means the user is done using this program. */
int done;

char *
dupstr (const char *s)
{
  char *r;

  r = malloc (strlen (s) + 1);
  if (!r)
    {
      fprintf (stderr, "Memory exhausted\n");
      exit (1);
    }
  strcpy (r, s);
  return r;
}


#ifdef WITH_READLINE
/* Interface to Readline Completion */

char *command_generator (const char *, int);
char **nntp_completion (char *, int, int);

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void
initialize_readline ()
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = (char *) "nntp";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = (CPPFunction *) nntp_completion;
}

/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
char **
nntp_completion (char *text, int start, int end)
{
  char **matches;

  (void) end;
  matches = (char **) NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = rl_completion_matches (text, command_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (const char *text, int state)
{
  static int list_index, len;
  const char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the command list. */
  while ((name = commands[list_index].name))
    {
      list_index++;

      if (strncmp (name, text, len) == 0)
	return (dupstr (name));
    }

  /* If no names matched, then return NULL. */
  return ((char *) NULL);
}

#else
void
initialize_readline ()
{
}

char *
readline (char *prompt)
{
  char buf[255];

  if (prompt)
    {
      printf ("%s", prompt);
      fflush (stdout);
    }

  if (!fgets (buf, sizeof (buf), stdin))
    return NULL;
  return strdup (buf);
}

void
add_history (const char *s ARG_UNUSED)
{
}
#endif


int
main (int argc ARG_UNUSED, char **argv)
{
  char *line, *s;

  progname = strrchr (argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  initialize_readline ();	/* Bind our completer. */

  /* Loop reading and executing lines until the user quits. */
  while (!done)
    {

      line = readline ((char *) "nntp> ");

      if (!line)
	break;

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite (line);

      if (*s)
	{
	  int status;
	  add_history (s);
	  status = execute_line (s);
	  if (status != 0)
	    fprintf (stderr, "Error: %s\n", mu_strerror (status));
	}

      free (line);
    }
  exit (0);
}

/* Parse and execute a command line. */
int
execute_line (char *line)
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && isspace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !isspace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command (word);

  if (!command)
    {
      fprintf (stderr, "%s: No such command for %s.\n", word, progname);
      return (-1);
    }

  /* Get argument to command, if any. */
  while (isspace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *
find_command (name)
     char *name;
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *) NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite (char *string)
{
  register char *s, *t;

  for (s = string; isspace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && isspace (*t))
    t--;
  *++t = '\0';

  return s;
}

int
com_verbose (char *arg)
{
  int status = 0;
  if (!valid_argument ("verbose", arg))
    return EINVAL;

  verbose = (strcmp (arg, "on") == 0);
  if (nntp != NULL)
    {
      if (verbose == 1)
	{
	  mu_debug_t debug;
	  mu_debug_create (&debug, NULL);
	  mu_debug_set_level (debug, MU_DEBUG_PROT);
	  status = mu_nntp_set_debug (nntp, debug);
	}
      else
	{
	  status = mu_nntp_set_debug (nntp, NULL);
	}
    }
  return status;
}

int
com_mode_reader (char *arg ARG_UNUSED)
{
  return mu_nntp_mode_reader (nntp);
}

int
com_head (char *arg)
{
  stream_t stream = NULL;
  int status;

  // No space allowed.
  if (arg != NULL && strchr (arg, ' ') != NULL)
    return EINVAL;

  if ((arg == NULL || *arg == '\0') || (arg != NULL && *arg == '<'))
    status = mu_nntp_head_id (nntp, arg, NULL, NULL, &stream);
  else
    {
      unsigned long number = strtoul (arg, NULL, 10);
      status = mu_nntp_head (nntp, number, NULL, NULL, &stream);
    }

   if (status == 0 && stream != NULL)
    {
      size_t n = 0;
      char buf[128];
      while ((stream_readline (stream, buf, sizeof buf, 0, &n) == 0) && n)
        printf ("%s", buf);
      stream_destroy (&stream, NULL);
    }
   return status;
}

int
com_body (char *arg)
{
  stream_t stream = NULL;
  int status;

  // No space allowed.
  if (arg != NULL && strchr (arg, ' ') != NULL)
    return EINVAL;

  if ((arg == NULL || *arg == '\0') || (arg != NULL && *arg == '<'))
    status = mu_nntp_body_id (nntp, arg, NULL, NULL, &stream);
  else
    {
      unsigned long number = strtoul (arg, NULL, 10);
      status = mu_nntp_body (nntp, number, NULL, NULL, &stream);
    }

   if (status == 0 && stream != NULL)
    {
      size_t n = 0;
      char buf[128];
      while ((stream_readline (stream, buf, sizeof buf, 0, &n) == 0) && n)
        printf ("%s", buf);
      stream_destroy (&stream, NULL);
    }
   return status;
}

int
com_article (char *arg)
{
  stream_t stream = NULL;
  int status;

  // No space allowed.
  if (arg != NULL && strchr (arg, ' ') != NULL)
    return EINVAL;

  if ((arg == NULL || *arg == '\0') || (arg != NULL && *arg == '<'))
    status = mu_nntp_body_id (nntp, arg, NULL, NULL, &stream);
  else
    {
      unsigned long number = strtoul (arg, NULL, 10);
      status = mu_nntp_body (nntp, number, NULL, NULL, &stream);
    }

   if (status == 0 && stream != NULL)
    {
      size_t n = 0;
      char buf[128];
      while ((stream_readline (stream, buf, sizeof buf, 0, &n) == 0) && n)
        printf ("%s", buf);
      stream_destroy (&stream, NULL);
    }
   return status;
}

int
com_group (char *arg)
{
  int status;
  unsigned long total, low, high;
  char *name = NULL;
  if (!valid_argument ("group", arg))
    return EINVAL;
  status = mu_nntp_group (nntp, arg, &total, &low, &high, &name);
  if (status == 0)
    {
      printf ("%s: low[%ld] high[%ld] total[%d]\n", (name == NULL) ? "" : name, low, high, total);
      free (name);
    }
  return status;
}

int
com_list_extensions (char *arg ARG_UNUSED)
{
  list_t list = NULL;
  int status = mu_nntp_list_extensions (nntp, &list);

  if (status == 0)
    {
      iterator_t iterator = NULL;
      iterator_create (&iterator, list);
      for (iterator_first (iterator);
	   !iterator_is_done (iterator); iterator_next (iterator))
	{
	  char *extension = NULL;
	  iterator_current (iterator, (void **) &extension);
	  printf ("Extension: %s\n", (extension) ? extension : "");
	}
      iterator_destroy (&iterator);
      list_destroy (&list);
    }
  return status;
}

int com_last (char *arg ARG_UNUSED)
{
  char *mid = NULL;
  unsigned long number = 0;
  int status;
  status = mu_nntp_last (nntp, &number, &mid);
  if (status == 0)
    {
      fprintf (stdout, "%ld %s\n", number, (mid == NULL) ? "" : mid);
      free (mid);
    }
  return status;
}

int com_next (char *arg ARG_UNUSED)
{
  char *mid = NULL;
  unsigned long number = 0;
  int status;
  status = mu_nntp_next (nntp, &number, &mid);
  if (status == 0)
    {
      fprintf (stdout, "%ld %s\n", number, (mid == NULL) ? "" : mid);
      free (mid);
    }
  return status;
}

int
com_stat (char *arg)
{
  char *mid = NULL;
  unsigned long number = 0;
  int status = 0;

  // No space allowed.
  if (arg != NULL && strchr (arg, ' ') != NULL)
    return EINVAL;

  if ((arg == NULL || *arg == '\0') || (arg != NULL && *arg == '<'))
    status = mu_nntp_stat_id (nntp, arg, &number, &mid);
  else
    {
      unsigned long number = strtoul (arg, NULL, 10);
      status = mu_nntp_stat (nntp, number, &number, &mid);
    }
  if (status == 0)
    {
      fprintf (stdout, "status: %d %s\n", number, (mid == NULL) ? "" : mid);
      free (mid);
    }
  return status;
}

int com_date (char *arg ARG_UNUSED)
{
  unsigned int year, month, day, hour, min, sec;
  int status;
  year = month = day = hour = min = sec = 0;
  status = mu_nntp_date (nntp, &year, &month, &day, &hour, &min, &sec);
  if (status == 0)
    {
      fprintf (stdout, "%d %d %d %d %d %d\n", year, month, day, hour, min, sec);
    }
  return status;
}

/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int
com_help (char *arg)
{
  int i;
  int printed = 0;

  for (i = 0; commands[i].name; i++)
    {
      if (!*arg || (strcmp (arg, commands[i].name) == 0))
	{
	  printf ("%s\t\t%s.\n", commands[i].name, commands[i].doc);
	  printed++;
	}
    }

  if (!printed)
    {
      printf ("No commands match `%s'.  Possibilties are:\n", arg);

      for (i = 0; commands[i].name; i++)
	{
	  /* Print in six columns. */
	  if (printed == 6)
	    {
	      printed = 0;
	      printf ("\n");
	    }

	  printf ("%s\t", commands[i].name);
	  printed++;
	}

      if (printed)
	printf ("\n");
    }
  return 0;
}

int
com_connect (char *arg)
{
  char host[256];
  int port = 110;
  int status;
  if (!valid_argument ("connect", arg))
    return 1;
  *host = '\0';
  sscanf (arg, "%256s %d", host, &port);
  if (!valid_argument ("connect", host))
    return EINVAL;
  if (nntp)
    com_disconnect (NULL);
  status = mu_nntp_create (&nntp);
  if (status == 0)
    {
      stream_t tcp;

      if (verbose)
	com_verbose ("verbose on");
      status =
	tcp_stream_create (&tcp, host, port,
			   MU_STREAM_READ | MU_STREAM_NO_CHECK);
      if (status == 0)
	{
	  mu_nntp_set_carrier (nntp, tcp);
	  status = mu_nntp_connect (nntp);
	}
      else
	{
	  mu_nntp_destroy (&nntp);
	  nntp = NULL;
	}
    }

  if (status != 0)
    fprintf (stderr, "Failed to create nntp: %s\n", mu_strerror (status));
  return status;
}

int
com_disconnect (char *arg)
{
  (void) arg;
  if (nntp)
    {
      mu_nntp_disconnect (nntp);
      mu_nntp_destroy (&nntp);
      nntp = NULL;
    }
  return 0;
}

int
com_quit (char *arg ARG_UNUSED)
{
  if (nntp)
    {
      if (mu_nntp_quit (nntp) == 0)
	{
	  mu_nntp_disconnect (nntp);
	}
      else
	{
	  fprintf (stdout, "Try 'exit' to leave %s\n", progname);
	}
    }
  else
    fprintf (stdout, "Try 'exit' to leave %s\n", progname);
  return 0;
}

int
com_exit (char *arg ARG_UNUSED)
{
  if (nntp)
    {
      mu_nntp_disconnect (nntp);
      mu_nntp_destroy (&nntp);
    }
  done = 1;
  return 0;
}

/* Return non-zero if ARG is a valid argument for CALLER, else print
   an error message and return zero. */
int
valid_argument (const char *caller, char *arg)
{
  if (arg == NULL || *arg == '\0' || strchr (arg, ' ') != NULL)
    {
      fprintf (stderr, "%s: Argument required.\n", caller);
      return 0;
    }

  return 1;
}
