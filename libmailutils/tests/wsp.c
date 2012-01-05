/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mailutils/wordsplit.h>
#include <mailutils/kwd.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/cstr.h>

extern char **environ;

struct mu_kwd bool_keytab[] = {
  { "append", MU_WRDSF_APPEND },
  /*{ "reuse",  MU_WRDSF_REUSE },*/
  { "undef",  MU_WRDSF_UNDEF },
  { "novar",  MU_WRDSF_NOVAR },
  { "nocmd",  MU_WRDSF_NOCMD },
  { "ws",     MU_WRDSF_WS },
  { "quote",  MU_WRDSF_QUOTE },
  { "squote", MU_WRDSF_SQUOTE },
  { "dquote", MU_WRDSF_DQUOTE },
  { "squeeze_delims", MU_WRDSF_SQUEEZE_DELIMS },
  { "return_delims",  MU_WRDSF_RETURN_DELIMS },
  { "sed",     MU_WRDSF_SED_EXPR },
  { "debug",   MU_WRDSF_SHOWDBG },
  { "nosplit",  MU_WRDSF_NOSPLIT },
  { "keepundef", MU_WRDSF_KEEPUNDEF },
  { "warnundef", MU_WRDSF_WARNUNDEF },
  { "cescapes", MU_WRDSF_CESCAPES },
  { "default", MU_WRDSF_DEFFLAGS },
  { "env_kv", MU_WRDSF_ENV_KV },
  { "incremental", MU_WRDSF_INCREMENTAL },
  { NULL, 0 }
};

struct mu_kwd string_keytab[] = {
  { "delim",   MU_WRDSF_DELIM },
  { "comment", MU_WRDSF_COMMENT },
  { "escape",  MU_WRDSF_ESCAPE },
  { NULL, 0 }
};

static void
help ()
{
  size_t i;
  
  printf ("usage: wsp [options]\n");
  printf ("options are:\n");
  printf (" [-]trimnl\n");
  printf (" [-]plaintext\n");
  putchar ('\n');
  for (i = 0; bool_keytab[i].name; i++)
    printf (" [-]%s\n", bool_keytab[i].name);
  putchar ('\n');
  for (i = 0; string_keytab[i].name; i++)
    {
      printf (" -%s\n", bool_keytab[i].name);
      printf (" %s ARG\n", bool_keytab[i].name);
    }
  putchar ('\n');
  printf (" -dooffs\n");
  printf (" dooffs COUNT ARGS...\n");
  exit (0);
}

void
print_qword (const char *word, int plaintext)
{
  static char *qbuf = NULL;
  static size_t qlen = 0;
  int quote;
  size_t size = mu_wordsplit_c_quoted_length (word, 0, &quote);

  if (plaintext)
    {
      printf ("%s", word);
      return;
    }

  if (*word == 0)
    quote = 1;
  
  if (size >= qlen)
    {
      qlen = size + 1;
      qbuf = realloc (qbuf, qlen);
      if (!qbuf)
	{
	  mu_error ("not enough memory");
	  abort ();
	}
    }
  mu_wordsplit_c_quote_copy (qbuf, word, 0);
  qbuf[size] = 0;
  if (quote)
    printf ("\"%s\"", qbuf);
  else
    printf ("%s", qbuf);
}

/* Convert environment to K/V form */
static char **
make_env_kv ()
{
  size_t i, j, size;
  char **newenv;
  
  /* Count the number of entries */
  for (i = 0; environ[i]; i++)
    ;

  size = (i - 1) * 2 + 1;
  newenv = calloc (size, sizeof (newenv[0]));
  if (!newenv)
    {
      mu_error ("not enough memory");
      exit (1);
    }

  for (i = j = 0; environ[i]; i++)
    {
      size_t len = strcspn (environ[i], "=");
      char *p = malloc (len+1);
      if (!p)
	{
	  mu_error ("not enough memory");
	  exit (1);
	}
      memcpy (p, environ[i], len);
      p[len] = 0;
      newenv[j++] = p;
      p = strdup (environ[i] + len + 1);
      if (!p)
	{
	  mu_error ("not enough memory");
	  exit (1);
	}
      newenv[j++] = p;
    }
  newenv[j] = NULL;
  return newenv;
}
    
int
main (int argc, char **argv)
{
  char buf[1024], *ptr;
  int i, offarg = 0;
  int trimnl_option = 0;
  int plaintext_option = 0;
  int wsflags = (MU_WRDSF_DEFFLAGS & ~MU_WRDSF_NOVAR) |
                 MU_WRDSF_ENOMEMABRT |
                 MU_WRDSF_ENV | MU_WRDSF_SHOWERR;
  struct mu_wordsplit ws;
  int next_call = 0;

  for (i = 1; i < argc; i++)
    {
      char *opt = argv[i];
      int negate;
      int flag;

      if (opt[0] == '-')
	{
	  negate = 1;
	  opt++;
	}
      else if (opt[0] == '+')
	{
	  negate = 0;
	  opt++;
	}
      else
	negate = 0;

      if (strcmp (opt, "h") == 0 ||
	  strcmp (opt, "help") == 0 ||
	  strcmp (opt, "-help") == 0)
	{
	  help ();
	}
	  
      if (strcmp (opt, "trimnl") == 0)
	{
	  trimnl_option = !negate;
	  continue;
	}

      if (strcmp (opt, "plaintext") == 0)
	{
	  plaintext_option = !negate;
	  continue;
	}
    
      if (mu_kwd_xlat_name (bool_keytab, opt, &flag) == 0)
	{
	  if (negate)
	    wsflags &= ~flag;
	  else
	    wsflags |= flag;
	  continue;
	}

      if (mu_kwd_xlat_name (string_keytab, opt, &flag) == 0)
	{
	  if (negate)
	    wsflags &= ~flag;
	  else
	    {
	      i++;
	      if (i == argc)
		{
		  mu_error ("%s missing argument", opt);
		  exit (1);
		}
	      
	      switch (flag)
		{
		case MU_WRDSF_DELIM:
		  ws.ws_delim = argv[i];
		  break;

		case MU_WRDSF_COMMENT:
		  ws.ws_comment = argv[i];
		  break;

		case MU_WRDSF_ESCAPE:
		  ws.ws_escape = argv[i];
		  break;
		}
	      
	      wsflags |= flag;
	    }
	  continue;
	}

      if (strcmp (opt, "dooffs") == 0)
	{
	  if (negate)
	    wsflags &= ~MU_WRDSF_DOOFFS;
	  else
	    {
	      char *p;

	      i++;
	      
	      if (i == argc)
		{
		  mu_error ("%s missing arguments", opt);
		  exit (1);
		}
	      ws.ws_offs = strtoul (argv[i], &p, 10);
	      if (*p)
		{
		  mu_error ("invalid number: %s", argv[i]);
		  exit (1);
		}

	      i++;
	      if (i + ws.ws_offs > argc)
		{
		  mu_error ("%s: not enough arguments", opt);
		  exit (1);
		}
	      offarg = i;
	      i += ws.ws_offs - 1;
	      wsflags |= MU_WRDSF_DOOFFS;
	    }
	  continue;
	}

      mu_error ("%s: unrecognized argument", opt);
      exit (1);
    }

  if (wsflags & MU_WRDSF_ENV_KV)
    ws.ws_env = (const char **) make_env_kv ();
  else
    ws.ws_env = (const char **) environ;

  if (wsflags & MU_WRDSF_INCREMENTAL)
    trimnl_option = 1;
  
  next_call = 0;
  while ((ptr = fgets (buf, sizeof (buf), stdin)))
    {
      int rc;
      size_t i;
      
      if (trimnl_option)
	mu_rtrim_cset (ptr, "\n");
      
      if (wsflags & MU_WRDSF_INCREMENTAL)
	{
	  if (next_call)
	    {
	      if (*ptr == 0)
		ptr = NULL;
	      else
		free ((void*)ws.ws_input);
	    }
	  else
	    next_call = 1;
	  if (ptr)
	    {
	      ptr = strdup (ptr);
	      if (!ptr)
		abort ();
	    }
	}
	
      rc = mu_wordsplit (ptr, &ws, wsflags);
      if (rc)
	{
	  if (!(wsflags & MU_WRDSF_SHOWERR))
	    mu_wordsplit_perror (&ws);
	  continue;
	}
	  
      if (offarg)
	{
	  for (i = 0; i < ws.ws_offs; i++)
	    ws.ws_wordv[i] = argv[offarg + i];
	  offarg = 0;
	}
      
      wsflags |= MU_WRDSF_REUSE;
      printf ("NF: %lu", (unsigned long) ws.ws_wordc);
      if (wsflags & MU_WRDSF_DOOFFS)
	printf (" (%lu)", (unsigned long) ws.ws_offs);
      putchar ('\n');
      for (i = 0; i < ws.ws_offs; i++)
	{
	  printf ("(%lu): ", (unsigned long) i);
	  print_qword (ws.ws_wordv[i], plaintext_option);
	  putchar ('\n');
	}
      for (; i < ws.ws_offs + ws.ws_wordc; i++)
	{
	  printf ("%lu: ", (unsigned long) i);
	  print_qword (ws.ws_wordv[i], plaintext_option);
	  putchar ('\n');
	}
    }
  return 0;
}
