/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <mailutils/mailutils.h>
#include <mailutils/tls.h>
#include "mailutils/libargp.h"
#include "muaux.h"
#include "mu.h"

#ifdef WITH_READLINE
# include <readline/readline.h>
# include <readline/history.h>
#endif

char *mutool_shell_prompt;
char **mutool_prompt_env;
int mutool_shell_interactive;


static int got_signal = 0;

static void
_shell_sig (int sig)
{
  got_signal = sig;
}

#define shell_interrupted() (got_signal == SIGINT)

static void
report_signals ()
{
  switch (got_signal)
    {
    case 0:
      break;

    case SIGINT:
      mu_stream_printf (mu_strerr, _("Interrupt\n"));
      mu_stream_flush (mu_strerr);

    default:
      got_signal = 0;
    }
}

static int shell_exit (int, char **);
static int shell_help (int, char **);
static int shell_prompt (int, char **);
#ifdef WITH_READLINE
static int shell_history (int, char **);
#endif

struct mutool_command default_comtab[] = {
  { "prompt",     1, 2, CMD_COALESCE_EXTRA_ARGS, shell_prompt,
    N_("STRING"),
    N_("set command prompt") },
  { "exit",       1, 1, 0, shell_exit,    NULL,        N_("exit program") },
  { "help",       1, 2, 0, shell_help,
    N_("[COMMAND]"),
    N_("display this text") },
  { "?",          1, 1, 0, shell_help,
    N_("[COMMAND]"),
    N_("synonym for `help'") },
#ifdef WITH_READLINE
  { "history",    1, 1, 0, shell_history,
    NULL,
    N_("show command history") },
#endif
  { NULL }
};

#define DESCRCOL 25
#define NUMCOLS  80
#define DESCRWIDTH (NUMCOLS - DESCRCOL)

/* Print a single comtab entry */
static void
print_comtab (mu_stream_t stream, struct mutool_command *tab)
{
  size_t size = 0;
  const char *text;
  
  if (tab->docstring == NULL)
    return;

  mu_stream_printf (stream, "%s ", tab->name);
  size += strlen (tab->name) + 1;
  if (tab->argdoc)
    {
      text = gettext (tab->argdoc);
      mu_stream_printf (stream, "%s", text);
      size += strlen (text);
    }
  if (size >= DESCRCOL)
    mu_stream_printf (stream, "\n%-*s", DESCRCOL, "");
  else
    mu_stream_printf (stream, "%-*s", (int) (DESCRCOL - size), "");

  text = gettext (tab->docstring);
  size = strlen (text);
  
  while (*text)
    {
      size_t len = size;
      if (len > DESCRWIDTH)
	{
	  /* Fold the text */
	  size_t n = 0;
	  
	  while (n < len)
	    {
	      size_t delta;
	      char *p;
	      
	      p = mu_str_skip_cset_comp (text + n, " \t");
	      delta = p - (text + n);
	      if (n + delta > DESCRWIDTH)
		break;
	      n += delta;
	      
	      p = mu_str_skip_cset (text + n, " \t");
	      delta = p - (text + n);
	      if (n + delta > DESCRWIDTH)
		break;
	      n += delta;
	    }

	  len = n;
	}
      mu_stream_write (stream, text, len, NULL);
      text += len;
      size -= len;
      mu_stream_write (stream, "\n", 1, NULL);
      if (size)
	mu_stream_printf (stream, "%-*s", DESCRCOL, "");
    }
}  

/* Print a single comtab entry.
   FIXME: Way too primitive. Rewrite. */
int
print_help (mu_stream_t stream, struct mutool_command *tab, size_t n)
{
  if (n)
    {
      for (; n > 0 && tab->name; tab++, n--)
	print_comtab (stream, tab);
    }
  else
    {
      for (; tab->name; tab++)
	print_comtab (stream, tab);
    }
  return 0;
}

/* Print all commands from TAB matching NAME.
   FIXME: Way too primitive. Rewrite. */
void
list_commands (struct mutool_command *tab, const char *name)
{
  size_t namelen = strlen (name);
  int printed = 0;
  
  for (; tab->name; tab++)
    {
      /* Print in six columns. */
      if (printed == 6)
	{
	  printed = 0;
	  mu_printf ("\n");
	}
      if (mu_c_strncasecmp (tab->name, name, namelen) == 0)
	{
	  mu_printf ("%s\t", tab->name);
	  printed++;
	}
    }
  if (printed && printed < 6)
    mu_printf ("\n");
}

static struct mutool_command *
simple_find_command (struct mutool_command *cp, const char *name)
{
  for (; cp->name; cp++)
    if (strcmp (cp->name, name) == 0)
      return cp;
  return NULL;
}


static struct mutool_command *shell_comtab;
static size_t user_command_count;

static struct mutool_command *
find_command (const char *name)
{
  return simple_find_command (shell_comtab, name);
}

/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int
shell_help (int argc, char **argv)
{
  char *name = argv[1];
  
  if (name)
    {
      struct mutool_command *com = find_command (argv[1]);

      if (com)
	print_comtab (mu_strout, com);
      else
	{
	  mu_printf ("No commands match `%s'.  Possibilties are:\n", name);
	  list_commands (shell_comtab, name);
	}
    }
  else
    {
      mu_stream_t str = mutool_open_pager ();
      print_help (str, shell_comtab, user_command_count);
      mu_stream_printf (str, "\n");
      print_help (str, shell_comtab + user_command_count, 0);
      mu_stream_destroy (&str);
    }
  return 0;
}

static int
shell_prompt (int argc, char **argv)
{
  size_t size;
  
  free (mutool_shell_prompt);
  size = strlen (argv[1]);
  mutool_shell_prompt = mu_alloc (size + 1);
  mu_wordsplit_c_unquote_copy (mutool_shell_prompt, argv[1], size);
  return 0;
}

mu_stream_t
mutool_open_pager ()
{
  char *pager;
  if (mutool_shell_interactive && (pager = getenv ("PAGER")) != NULL)
    {
      mu_stream_t stream;
      int rc = mu_command_stream_create (&stream, pager, MU_STREAM_WRITE);
      if (rc == 0)
	return stream;
      mu_error (_("cannot start pager: %s"), mu_strerror (rc));
    }
  mu_stream_ref (mu_strout);
  return mu_strout;
}


#ifdef WITH_READLINE
#define HISTFILE_PREFIX "~/.mu_"
#define HISTFILE_SUFFIX "_history"

static char *
get_history_file_name ()
{
  static char *filename = NULL;

  if (!filename)
    {
      char *hname;
      
      hname = mu_alloc (sizeof HISTFILE_PREFIX + strlen (rl_readline_name) +
		        sizeof HISTFILE_SUFFIX - 1);
      strcpy (hname, "~/.mu_");
      strcat (hname, rl_readline_name);
      strcat (hname, HISTFILE_SUFFIX);
      filename = mu_tilde_expansion (hname, MU_HIERARCHY_DELIMITER, NULL);
      free(hname);
    }
  return filename;
}

static int
_shell_getc (FILE *stream)
{
  unsigned char c;

  while (1)
    {
      if (read (fileno (stream), &c, 1) == 1)
	return c;
      if (errno == EINTR)
	{
	  if (shell_interrupted ())
	    break;
	  /* keep going if we handled the signal */
	}
      else
	break;
    }
  return EOF;
}

/* Interface to Readline Completion */

static char *shell_command_generator (const char *, int);
static char **shell_completion (char *, int, int);

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void
mutool_initialize_readline (const char *name)
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = (char *) name;
  rl_attempted_completion_function = (CPPFunction *) shell_completion;
  rl_getc_function = _shell_getc;
  read_history (get_history_file_name ());
}

static void
finish_readline ()
{
  write_history (get_history_file_name());
}

/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
static char **
shell_completion (char *text, int start, int end MU_ARG_UNUSED)
{
  char **matches;

  matches = (char **) NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches = rl_completion_matches (text, shell_command_generator);

  return (matches);
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
static char *
shell_command_generator (const char *text, int state)
{
  const char *name;
  static int len;
  static struct mutool_command *cmd;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      cmd = shell_comtab;
      len = strlen (text);
    }

  if (!cmd->name)
    return NULL;
    
  
  /* Return the next name which partially matches from the command list. */
  while ((name = cmd->name))
    {
      cmd++;
      if (strncmp (name, text, len) == 0)
	return mu_strdup (name);
    }

  /* If no names matched, then return NULL. */
  return NULL;
}

static char *pre_input_line;

static int
pre_input (void)
{
  rl_insert_text (pre_input_line);
  free (pre_input_line);
  rl_pre_input_hook = NULL;
  rl_redisplay ();
  return 0;
}

static int
retrieve_history (char *str)
{
  char *out;
  int rc;

  rc = history_expand (str, &out);
  switch (rc)
    {
    case -1:
      mu_error ("%s", out);
      free (out);
      return 1;

    case 0:
      break;

    case 1:
      pre_input_line = out;
      rl_pre_input_hook = pre_input;
      return 1;

    case 2:
      mu_printf ("%s\n", out);
      free (out);
      return 1;
    }
  return 0;
}

static int
shell_history (int argc, char **argv)
{
  int i;
  HIST_ENTRY **hlist;
  mu_stream_t out = mutool_open_pager ();
  
  hlist = history_list ();
  for (i = 0; i < history_length; i++)
    mu_stream_printf (out, "%4d) %s\n", i + 1, hlist[i]->line);

  mu_stream_destroy (&out);
  return 0;
}

#else
# define finish_readline()
# define mutool_initialize_readline(name)

char *
readline (char *prompt)
{
  static size_t size = 0;
  static char *buf = NULL;
  size_t n;

  if (prompt)
    {
      mu_printf ("%s", prompt);
      mu_stream_flush (mu_strout);
    }
  if (mu_stream_getline (mu_strin, &buf, &size, &n) || n == 0)
    {
      free (buf);
      buf = NULL;
      size = 0;
      return NULL;
    }
  return buf;
}

void
add_history (const char *s MU_ARG_UNUSED)
{
}
#endif

static int
next_arg (struct mu_wordsplit *ws)
{
  int rc = mu_wordsplit (NULL, ws, MU_WRDSF_INCREMENTAL);
  if (rc == MU_WRDSE_NOINPUT)
    {
      mu_error ("%s: too few arguments", ws->ws_wordv[0]);
      mu_wordsplit_free (ws);
      return -1;
    }
  else if (rc)
    {
      mu_error ("cannot parse input line: %s",
		mu_wordsplit_strerror (ws));
      return 1;
    }
  return 0;
}

/* Parse and execute a command line. */
int
execute_line (char *line)
{
  int rc;
  struct mu_wordsplit ws;
  struct mutool_command *cmd;
  int status = 0;
  
  ws.ws_comment = "#";
  ws.ws_escape = "\\\"";
  rc = mu_wordsplit (line, &ws,
		     MU_WRDSF_DEFFLAGS|MU_WRDSF_COMMENT|MU_WRDSF_ESCAPE|
		     MU_WRDSF_INCREMENTAL|MU_WRDSF_APPEND);
  if (rc == MU_WRDSE_NOINPUT)
    {
      mu_wordsplit_free (&ws);
      return 0;
    }
  else if (rc)
    {
      mu_error ("cannot parse input line: %s", mu_wordsplit_strerror (&ws));
      return 0;
    }
  
  if (ws.ws_wordc)
    {
      int argmin;
      cmd = find_command (ws.ws_wordv[0]);

      if (!cmd)
	{
	  mu_error ("%s: no such command.", ws.ws_wordv[0]);
	  mu_wordsplit_free (&ws);
	  return 0;
	}

      argmin = cmd->argmin;
      if (cmd->flags & CMD_COALESCE_EXTRA_ARGS)
	--argmin;
      while (ws.ws_wordc < argmin)
	{
	  if (next_arg (&ws))
	    return 0;
	}

      if (cmd->flags & CMD_COALESCE_EXTRA_ARGS)
	{
	  ws.ws_flags |= MU_WRDSF_NOSPLIT;
	  if (next_arg (&ws))
	    return 0;
	}
      else
	{
	  for (;;)
	    {
	      if (cmd->argmax > 0 && ws.ws_wordc > cmd->argmax)
		{
		  mu_error ("%s: too many arguments", ws.ws_wordv[0]);
		  mu_wordsplit_free (&ws);
		  return 0;
		}
	      rc = mu_wordsplit (NULL, &ws, MU_WRDSF_INCREMENTAL);
	      if (rc == 0)
		continue;
	      else if (rc == MU_WRDSE_NOINPUT)
		break;
	      else
		{
		  mu_error ("cannot parse input line: %s",
			    mu_wordsplit_strerror (&ws));
		  return 0;
		}
	    }
	}

      status = cmd->func (ws.ws_wordc, ws.ws_wordv);
    }
  mu_wordsplit_free (&ws);
  return status;
}

static char *
input_line_interactive ()
{
  char *line;
  int wsflags = MU_WRDSF_NOSPLIT | MU_WRDSF_NOCMD;
  struct mu_wordsplit ws;
  
  report_signals ();
  if (mutool_prompt_env)
    {
      ws.ws_env = (const char **)mutool_prompt_env;
      wsflags |= MU_WRDSF_ENV | MU_WRDSF_ENV_KV;
    }
  if (mu_wordsplit (mutool_shell_prompt, &ws, wsflags))
    line = readline (mutool_shell_prompt);
  else
    {
      line = readline (ws.ws_wordv[0]);
      mu_wordsplit_free (&ws);
    }
  return line;
}

static char *
input_line_script ()
{
  size_t size = 0, n;
  char *buf = NULL;

  report_signals ();
  if (mu_stream_getline (mu_strin, &buf, &size, &n) || n == 0)
    return NULL;
  return buf;
}

static int done;

static int
shell_exit (int argc MU_ARG_UNUSED, char **argv MU_ARG_UNUSED)
{
  done = 1;
  return 0;
}

int
mutool_shell (const char *name, struct mutool_command *cmd)
{
  size_t n;
  char *(*input_line) ();
  static int sigv[] = { SIGPIPE, SIGINT };
				   
  mutool_shell_interactive = isatty (0);
  input_line = mutool_shell_interactive ?
                             input_line_interactive : input_line_script;

  for (n = 0; cmd[n].name; n++)
    ;

  user_command_count = n;
  shell_comtab = mu_calloc (n + MU_ARRAY_SIZE (default_comtab),
			    sizeof (shell_comtab[0]));
  memcpy (shell_comtab, cmd, n * sizeof (shell_comtab[0]));
  memcpy (shell_comtab + n, default_comtab, sizeof (default_comtab));

  mutool_initialize_readline (name);
  mu_set_signals (_shell_sig, sigv, MU_ARRAY_SIZE (sigv));
  while (!done)
    {
      char *s, *line = input_line ();
      if (!line)
	{
	  if (shell_interrupted ())
	    {
	      report_signals ();
	      continue;
	    }
	  else
	    {
	      mu_printf ("\n");
	      break;
	    }
	}
      
      /* Remove leading and trailing whitespace from the line.
         Then, if there is anything left, add it to the history list
         and execute it. */
      s = mu_str_stripws (line);

      if (*s)
	{
	  int status;

#ifdef WITH_READLINE
	  if (mutool_shell_interactive)
	    {
	      if (retrieve_history (s))
		continue;
	      add_history (s);
	    }
#endif

	  status = execute_line (s);
	  if (status != 0)
	    mu_error ("Error: %s", mu_strerror (status));
	}

      free (line);
    }
  if (mutool_shell_interactive)
    finish_readline ();
  mu_stream_destroy (&mu_strin);
  mu_stream_destroy (&mu_strout);
  return 0;
}

