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

static char **ml_command_completion __P((char *cmd, int start, int end));
static char *ml_command_generator __P((char *text, int state));

static int _interrupt;

static RETSIGTYPE
sig_handler (int signo)
{
  switch (signo)
    {
    case SIGINT:
      if (util_find_env ("quit")->set)
	exit (0);
      _interrupt++;
      break;
#if defined (SIGWINCH)
    case SIGWINCH:
      break;
#endif
    }
  signal (signo, sig_handler);
}

void
ml_clear_interrupt ()
{
  _interrupt = 0;
}

int
ml_got_interrupt ()
{
  int rc = _interrupt;
  _interrupt = 0;
  return rc;
}

#ifdef WITH_READLINE
int
ml_getc (FILE *stream)
{
    unsigned char c;

    while (1)
      {
	if (read (fileno (stream), &c, 1) == 1)
	  return c;
	if (errno == EINTR)
	  {
	    if (_interrupt)
	      break;
	    /* keep going if we handled the signal */
	  }
	else
	  break;
      }
    return EOF;
}
#endif

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
  signal (SIGINT, sig_handler);
#if defined(SIGWINCH)      
  signal (SIGWINCH, sig_handler);
#endif  
}

#ifdef WITH_READLINE

static char *insert_text;

static int
ml_insert_hook ()
{
  if (insert_text)
    rl_insert_text (insert_text);
  return 0;
}

int
ml_reread (char *prompt, char **text)
{
  char *s;
  
  insert_text = *text;
  rl_startup_hook = ml_insert_hook;
  s = readline (prompt);
  if (*text)
    free (*text);
  *text = s;
  rl_startup_hook = NULL;
  return 0;
}

/*
 * readline tab completion
 */
char **
ml_command_completion (char *cmd, int start, int end)
{
  if (start == 0)
    return completion_matches (cmd, ml_command_generator);
  return NULL;
}

/*
 * more readline
 */
char *
ml_command_generator (char *text, int state)
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

int
ml_reread (char *prompt, char **text)
{
  char *s;
  /*FIXME*/
  s = readline (prompt);
  if (*text)
    free (*text);
  *text = s;
  return 0;
}

char *
readline (const char *prompt)
{
  char *line;
  char *p;
  size_t alloclen, linelen;

  if (prompt)
    {
      fprintf (ofile, "%s", prompt);
      fflush (ofile);
    }

  p = line = calloc (1, 255);
  alloclen = 255;
  linelen = 0;
  for (;;)
    {
      size_t n;

      p = fgets (p, alloclen - linelen, stdin);
      
      if (p)
	n = strlen(p);
      else if (_interrupt)
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
#endif

