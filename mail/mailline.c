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

#ifdef WITH_READLINE
static char **ml_command_completion __P((char *cmd, int start, int end));
static char *ml_command_generator __P((const char *text, int state));
#endif

static volatile int _interrupted;

static RETSIGTYPE
sig_handler (int signo)
{
  switch (signo)
    {
    case SIGINT:
      if (util_getenv (NULL, "quit", Mail_env_boolean, 0) == 0)
	exit (0);
      _interrupted++;
      break;
#if defined (SIGWINCH)
    case SIGWINCH:
      util_do_command ("set screen=%d", util_getlines());
      util_do_command ("set columns=%d", util_getcols());
      break;
#endif
    }
#ifndef HAVE_SIGACTION
  signal (signo, sig_handler);
#endif
}

void
ml_clear_interrupt ()
{
  _interrupted = 0;
}

int
ml_got_interrupt ()
{
  int rc = _interrupted;
  _interrupted = 0;
  return rc;
}

static int
ml_getc (FILE *stream)
{
    unsigned char c;

    while (1)
      {
	if (read (fileno (stream), &c, 1) == 1)
	  return c;
	if (errno == EINTR)
	  {
	    if (_interrupted)
	      break;
	    /* keep going if we handled the signal */
	  }
	else
	  break;
      }
    return EOF;
}

void
ml_readline_init ()
{
  if (!interactive)
    return;

#ifdef WITH_READLINE
  rl_readline_name = "mail";
  rl_attempted_completion_function = (CPPFunction*)ml_command_completion;
  rl_getc_function = ml_getc;
#endif
#ifdef HAVE_SIGACTION
  {
    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    sigaction (SIGINT, &act, NULL);
#if defined(SIGWINCH)
    sigaction (SIGWINCH, &act, NULL);
#endif
  }
#else
  signal (SIGINT, sig_handler);
#if defined(SIGWINCH)
  signal (SIGWINCH, sig_handler);
#endif
#endif
}

char *
ml_readline_internal ()
{
  char *line;
  char *p;
  size_t alloclen, linelen;

  p = line = xcalloc (1, 255);
  alloclen = 255;
  linelen = 0;
  for (;;)
    {
      size_t n;

      p = fgets (p, alloclen - linelen, stdin);

      if (p)
	n = strlen(p);
      else if (_interrupted)
	{
	  free (line);
	  return NULL;
	}
      else
	n = 0;

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

char *
ml_readline (const char *prompt)
{
  if (interactive)
    return readline (prompt);
  return ml_readline_internal ();
}


#ifdef WITH_READLINE

static char *insert_text;

static int
ml_insert_hook (void)
{
  if (insert_text)
    rl_insert_text (insert_text);
  return 0;
}

int
ml_reread (const char *prompt, char **text)
{
  char *s;

  ml_clear_interrupt ();
  insert_text = *text;
  rl_startup_hook = ml_insert_hook;
  s = readline ((char *)prompt);
  if (!ml_got_interrupt ())
    {
      if (*text)
	free (*text);
      *text = s;
    }
  else
    {
      putc('\n', stdout);
    }
  rl_startup_hook = NULL;
  return 0;
}

/*
 * readline tab completion
 */
char **
ml_command_completion (char *cmd, int start, int end)
{
  (void)end;
  if (start == 0)
    return rl_completion_matches (cmd, ml_command_generator);
  return NULL;
}

/*
 * more readline
 */
char *
ml_command_generator (const char *text, int state)
{
  static int i, len;
  const char *name;

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

#include <sys/ioctl.h>

#define STDOUT 1
#ifndef TIOCSTI
static int ch_erase;
static int ch_kill;
#endif

#if defined(TIOCSTI)
#elif defined(HAVE_TERMIOS_H)
# include <termios.h>

static struct termios term_settings;

int
set_tty ()
{
  struct termios new_settings;

  if (tcgetattr(STDOUT, &term_settings) == -1)
    return 1;

  ch_erase = term_settings.c_cc[VERASE];
  ch_kill = term_settings.c_cc[VKILL];

  new_settings = term_settings;
  new_settings.c_lflag &= ~(ICANON|ECHO);
#if defined(TAB3)
  new_settings.c_oflag &= ~(TAB3);
#elif defined(OXTABS)
  new_settings.c_oflag &= ~(OXTABS);
#endif
  new_settings.c_cc[VMIN] = 1;
  new_settings.c_cc[VTIME] = 0;
  tcsetattr(STDOUT, TCSADRAIN, &new_settings);
  return 0;
}

void
restore_tty ()
{
  tcsetattr (STDOUT, TCSADRAIN, &term_settings);
}

#elif defined(HAVE_TERMIO_H)
# include <termio.h>

static struct termio term_settings;

int
set_tty ()
{
  struct termio new_settings;

  if (ioctl(STDOUT, TCGETA, &term_settings) < 0)
    return -1;

  ch_erase = term_settings.c_cc[VERASE];
  ch_kill = term_settings.c_cc[VKILL];

  new_settings = term_settings;
  new_settings.c_lflag &= ~(ICANON | ECHO);
  new_settings.c_oflag &= ~(TAB3);
  new_settings.c_cc[VMIN] = 1;
  new_settings.c_cc[VTIME] = 0;
  ioctl(STDOUT, TCSETA, &new_settings);
  return 0;
}

void
restore_tty ()
{
  ioctl(STDOUT, TCSETA, &term_settings);
}

#elif defined(HAVE_SGTTY_H)
# include <sgtty.h>

static struct sgttyb term_settings;

int
set_tty ()
{
  struct sgttyb new_settings;

  if (ioctl(STDOUT, TIOCGETP, &term_settings) < 0)
    return 1;

  ch_erase = term_settings.sg_erase;
  ch_kill = term_settings.sg_kill;

  new_settings = term_settings;
  new_settings.sg_flags |= CBREAK;
  new_settings.sg_flags &= ~(ECHO | XTABS);
  ioctl(STDOUT, TIOCSETP, &new_settings);
  return 0;
}

void
restore_tty ()
{
  ioctl(STDOUT, TIOCSETP, &term_settings);
}

#else
# define DUMB_MODE
#endif


#define LINE_INC 80

int
ml_reread (const char *prompt, char **text)
{
  int ch;
  char *line;
  int line_size;
  int pos;
  char *p;

  if (*text)
    {
      line = strdup (*text);
      if (line)
	{
	  pos = strlen(line);
	  line_size = pos + 1;
	}
    }
  else
    {
      line_size = LINE_INC;
      line = malloc (line_size);
      pos = 0;
    }

  if (!line)
    {
      util_error("not enough memory to edit the line");
      return -1;
    }

  line[pos] = 0;

  if (prompt)
    {
      fputs (prompt, stdout);
      fflush (stdout);
    }

#ifdef TIOCSTI

  for (p = line; *p; p++)
    {
      ioctl(0, TIOCSTI, p);
    }

  pos = 0;

  while ((ch = ml_getc (stdin)) != EOF && ch != '\n')
    {
      if (pos >= line_size)
	{
	  if ((p = realloc (line, line_size + LINE_INC)) == NULL)
	    {
	      fputs ("\n", stdout);
	      util_error ("not enough memory to edit the line");
	      break;
	    }
	  else
	    {
	      line_size += LINE_INC;
	      line = p;
	    }
	}
      line[pos++] = ch;
    }

#else

  fputs (line, stdout);
  fflush (stdout);

# ifndef DUMB_MODE
  set_tty ();

  while ((ch = ml_getc (stdin)) != EOF)
    {
      if (ch == ch_erase)
	{
	  /* kill last char */
	  if (pos > 0)
	    line[--pos] = 0;
	  putc('\b', stdout);
	}
      else if (ch == ch_kill)
	{
	  /* kill the entire line */
	  pos = 0;
	  line[pos] = 0;
	  putc ('\r', stdout);
	  if (prompt)
	    fputs (prompt, stdout);
	}
      else if (ch == '\n' || ch == EOF)
	break;
      else
	{
	  if (pos >= line_size)
	    {
	      if ((p = realloc (line, line_size + LINE_INC)) == NULL)
		{
		  fputs("\n", stdout);
		  util_error("not enough memory to edit the line");
		  break;
		}
	      else
		{
		  line_size += LINE_INC;
		  line = p;
		}
	    }
	  line[pos++] = ch;
	  putc(ch, stdout);
	}
      fflush (stdout);
    }

  putc ('\n', stdout);
  restore_tty ();
# else
  /* Dumb mode: the last resort */

  putc ('\n', stdout);
  if (prompt)
    {
      fputs (prompt, stdout);
      fflush (stdout);
    }

  pos = 0;

  while ((ch = ml_getc (stdin)) != EOF && ch != '\n')
    {
      if (pos >= line_size)
	{
	  if ((p = realloc (line, line_size + LINE_INC)) == NULL)
	    {
	      fputs ("\n", stdout);
	      util_error ("not enough memory to edit the line");
	      break;
	    }
	  else
	    {
	      line_size += LINE_INC;
	      line = p;
	    }
	}
      line[pos++] = ch;
    }
# endif
#endif

  line[pos] = 0;

  if (ml_got_interrupt ())
    free (line);
  else
    {
      if (*text)
	free (*text);
      *text = line;
    }
  return 0;
}

char *
readline (const char *prompt)
{
  if (prompt)
    {
      fprintf (ofile, "%s", prompt);
      fflush (ofile);
    }

  return ml_readline_internal ();
}
#endif

