/* pop3client.c -- An application which demonstrates how to use the
   GNU mailutils pop3 functions.  This application interactively allows users
   to contact a pop3 server. */

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

#include <readline/readline.h>
#include <readline/history.h>

#include <mailutils/pop3.h>

/* A structure which contains information on the commands this program
   can understand. */

typedef struct {
  const char *name;		/* User printable name of the function. */
  int (*func) (char *);	/* Function to call to do the job. */
  const char *doc;	/* Documentation for this function.  */
} COMMAND;

/* The names of functions that actually do the manipulation. */
int com_apop (char *);
int com_disconnect (char *);
int com_dele (char *);
int com_exit (char *);
int com_help (char *);
int com_list (char *);
int com_noop (char *);
int com_connect (char *);
int com_pass (char *);
int com_quit (char *);
int com_retr (char *);
int com_rset (char *);
int com_stat (char *);
int com_top  (char *);
int com_uidl (char *);
int com_user (char *);
int print_response (void);

void initialize_readline (void);
char *stripwhite (char *);
COMMAND *find_command (char *);
void *xmalloc (size_t);
char *dupstr (const char *);
int execute_line (char *);
int valid_argument (const char *, char *);

void sig_int (int);

COMMAND commands[] = {
  { "apop", com_apop, "Authenticate with APOP: APOP user secret" },
  { "disconnect", com_disconnect, "Close connection: disconnect" },
  { "dele", com_dele, "Mark message: DELE msgno" },
  { "exit", com_exit, "exit program" },
  { "help", com_help, "Display this text" },
  { "?",    com_help, "Synonym for `help'" },
  { "list", com_list, "List messages: LIST [msgno]" },
  { "noop", com_noop, "Send no operation: NOOP" },
  { "pass", com_pass, "Send passwd: PASS [passwd]" },
  { "connect", com_connect, "Open connection: connect hostname [port]" },
  { "quit", com_quit, "Go to Update state : QUIT" },
  { "retr", com_retr, "Dowload message: RETR msgno" },
  { "rset", com_rset, "Unmark all messages: RSET" },
  { "stat", com_stat, "Get the size and count of mailbox : STAT [msgno]" },
  { "top",  com_top,  "Get the header of message: TOP msgno [lines]" },
  { "uidl", com_uidl, "Get the uniq id of message: UIDL [msgno]" },
  { "user", com_user, "send login: USER user" },
  { NULL, NULL, NULL }
};

/* The name of this program, as taken from argv[0]. */
char *progname;

/* Global handle for pop3.  */
pop3_t pop3;

/* When non-zero, this global means the user is done using this program. */
int done;

void *
xmalloc (size_t size)
{
  void *m = malloc (size);
  if (!m)
    {
      fprintf (stderr, "Memory exhausted\n");
      exit (1);
    }
  return m;
}

char *
dupstr (const char *s)
{
  char *r;

  r = xmalloc (strlen (s) + 1);
  strcpy (r, s);
  return r;
}

int
main (int argc, char **argv)
{
  char *line, *s;

  (void)argc;
  progname = argv[0];

  initialize_readline ();	/* Bind our completer. */

  /* Loop reading and executing lines until the user quits. */
  for ( ; done == 0; )
    {

      line = readline ((char *)"pop3> ");

      if (!line)
        break;

      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = stripwhite (line);

      if (*s)
        {
          add_history (s);
          execute_line (s);
        }

      free (line);
    }
  exit (0);
}

/* Execute a command line. */
int
execute_line (char *line)
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
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
  while (whitespace (line[i]))
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

  return ((COMMAND *)NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite (char *string)
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

char *command_generator (const char *, int);
char **pop_completion (char *, int, int);

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void
initialize_readline ()
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = (char *)"pop3";

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = (CPPFunction *)pop_completion;
}

/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
char **
pop_completion (char *text, int start, int end)
{
  char **matches;

  (void)end;
  matches = (char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = completion_matches (text, command_generator);

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
        return (dupstr(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

int
print_response ()
{
  char response[1024];
  if (pop3)
    {
      pop3_response (pop3, response, sizeof response, NULL);
      fprintf (stderr, "%s\n", response);
    }
  else
    fprintf (stderr, "Not connected, try `connect' first\n");
  return 0;
}

int
com_user (char *arg)
{
  if (!valid_argument ("user", arg))
    return 1;
  pop3_user (pop3, arg);
  return print_response ();
}
int
com_apop (char *arg)
{
  char *user, *digest;

  if (!valid_argument ("apop", arg))
    return 1;
  user = strtok (arg, " ");
  digest = strtok (NULL, " ");
  if (!valid_argument ("apop", user) || !valid_argument ("apop", digest))
    return 1;
  pop3_apop (pop3, user, digest);
  return print_response ();
}

int
com_uidl (char *arg)
{
  if (arg == NULL || *arg == '\0')
    {
      iterator_t uidl_iterator;
      int status = pop3_uidl_all (pop3, &uidl_iterator);
      print_response ();
      if (status == 0)
	{
	  for (iterator_first (uidl_iterator);
	       !iterator_is_done (uidl_iterator);
	       iterator_next (uidl_iterator))
	    {
	      struct pop3_uidl_item *pl;
	      iterator_current (uidl_iterator, (void *)&pl);
	      printf ("Msg: %d UIDL: %s\n", pl->msgno, pl->uidl);
	      free (pl);
	    }
	  iterator_destroy (uidl_iterator);
	}
    }
  else
    {
      char *uidl = NULL;
      unsigned int msgno = strtoul (arg, NULL, 10);
      if (pop3_uidl (pop3, msgno, &uidl) == 0)
	printf ("Msg: %d UIDL: %s\n", msgno, (uidl) ? uidl : "");
      else
	print_response ();
    }
  return 0;
}

int
com_list (char *arg)
{
  if (arg == NULL || *arg == '\0')
    {
      iterator_t list_iterator;
      int status = pop3_list_all (pop3, &list_iterator);
      print_response ();
      if (status == 0)
	{
	  for (iterator_first (list_iterator);
	       !iterator_is_done (list_iterator);
	       iterator_next (list_iterator))
	    {
	      struct pop3_list_item *pl;
	      iterator_current (list_iterator, (void *)&pl);
	      printf ("Msg: %d Size: %d\n", pl->msgno, pl->size);
	      free (pl);
	    }
	  iterator_destroy (list_iterator);
	}
    }
  else
    {
      size_t size = 0;
      unsigned int msgno = strtoul (arg, NULL, 10);
      if (pop3_list (pop3, msgno, &size) == 0)
	printf ("Msg: %d Size: %d\n", msgno, size);
      else
	print_response ();
    }
  return 0;
}

int
com_noop (char *arg)
{
  (void)arg;
  pop3_noop (pop3);
  return print_response ();
}

static void
echo_off(struct termios *stored_settings)
{
  struct termios new_settings;
  tcgetattr (0, stored_settings);
  new_settings = *stored_settings;
  new_settings.c_lflag &= (~ECHO);
  tcsetattr (0, TCSANOW, &new_settings);
}

static void
echo_on(struct termios *stored_settings)
{
  tcsetattr (0, TCSANOW, stored_settings);
}

int
com_pass (arg)
     char *arg;
{
  char pass[256];
  if (!arg || *arg == '\0')
    {
      struct termios stored_settings;

      printf ("passwd:");
      fflush (stdout);
      echo_off (&stored_settings);
      fgets (pass, sizeof pass, stdin);
      echo_on (&stored_settings);
      putchar ('\n');
      fflush (stdout);
      pass [strlen (pass) - 1] = '\0'; /* nuke the trailing line.  */
      arg = pass;
    }
  pop3_pass (pop3, arg);
  return print_response ();
}

int
com_stat (char *arg)
{
  unsigned count, size;

  (void)arg;
  count = size = 0;
  pop3_stat (pop3, &count, &size);
  print_response ();
  fprintf (stdout, "Mesgs: %d Size %d\n", count, size);
  return 0;
}

int
com_dele (arg)
     char *arg;
{
  unsigned msgno;
  if (!valid_argument ("dele", arg))
      return 1;
  msgno = strtoul (arg, NULL, 10);
  pop3_dele (pop3, msgno);
  return print_response ();
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
com_rset (char *arg)
{
  (void)arg;
  pop3_rset (pop3);
  return print_response ();
}

int
com_top (char *arg)
{
  stream_t stream;
  unsigned int msgno;
  unsigned int lines;
  char *space;
  int status;

  if (!valid_argument ("top", arg))
    return 1;

  space = strchr (arg, ' ');
  if (space)
    {
      *space++ = '\0';
      lines = strtoul (space, NULL, 10);
    }
  else
    lines = 0;
  msgno = strtoul (arg, NULL, 10);

  status = pop3_top (pop3, msgno, lines, &stream);
  print_response ();

  if (status == 0)
    {
      size_t n = 0;
      char buf[128];
      while ((stream_readline (stream, buf, sizeof buf, &n) == 0)  && n)
	printf ("%s", buf);
      stream_destroy (stream);
    }
  return 0;
}

int
com_retr (char *arg)
{
  stream_t stream;
  unsigned int msgno;
  int status;

  if (!valid_argument ("top", arg))
    return 1;

  msgno = strtoul (arg, NULL, 10);
  status = pop3_retr (pop3, msgno, &stream);
  print_response ();

  if (status == 0)
    {
      size_t n = 0;
      char buf[128];
      while ((stream_readline (stream, buf, sizeof buf, &n) == 0)  && n)
	printf ("%s", buf);
      stream_destroy (stream);
    }
  return 0;
}

int
com_connect (char *arg)
{
  char host[256];
  int port = 0;
  int status;
  if (!valid_argument ("connect", arg))
    return 1;
  *host = '\0';
  sscanf (arg, "%256s %d", host, &port);
  if (!valid_argument ("connect", host))
    return 1;
  if (pop3)
    com_disconnect (NULL);
  status = pop3_create (&pop3);
  if (status == 0)
    {
      pop3_connect (pop3, host, port);
      print_response ();
    }
  else
    fprintf (stderr, "Failed to create pop3: %s\n", strerror (status));
  return 0;
}

int
com_disconnect (char *arg)
{
  (void) arg;
  if (pop3)
    {
      pop3_disconnect (pop3);
      pop3_destroy (pop3);
      pop3 = NULL;
    }
  return 0;
}

int
com_quit (char *arg)
{
  (void)arg;
  if (pop3)
    {
      if (pop3_quit (pop3) == 0)
	{
	  print_response ();
	  pop3_disconnect (pop3);
	}
      else
	{
	  print_response ();
	  fprintf (stdout, "Try 'exit' to leave %s\n", progname);
	}
    }
  else
    fprintf (stdout, "Try 'exit' to leave %s\n", progname);
  return 0;
}

int
com_exit (char *arg)
{
  (void)arg;
  if (pop3)
    {
      pop3_disconnect (pop3);
      pop3_destroy (pop3);
    }
  done = 1;
  return 0;
}

/* Return non-zero if ARG is a valid argument for CALLER, else print
   an error message and return zero. */
int
valid_argument (const char *caller, char *arg)
{
  if (!arg || !*arg)
    {
      fprintf (stderr, "%s: Argument required.\n", caller);
      return 0;
    }

  return 1;
}
