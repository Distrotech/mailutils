/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007, 2009-2012, 2014-2016 Free Software
   Foundation, Inc.

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
#include <mailutils/alloc.h>
#include <mailutils/kwd.h>
#include <mailutils/errno.h>
#include <mailutils/error.h>
#include <mailutils/cstr.h>
#include <mailutils/io.h>

extern char **environ;

char *progname;

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
  { "pathexpand", MU_WRDSF_PATHEXPAND },
  { NULL, 0 }
};

struct mu_kwd opt_keytab[] = {
  { "nullglob", MU_WRDSO_NULLGLOB },
  { "failglob", MU_WRDSO_FAILGLOB },
  { "dotglob",  MU_WRDSO_DOTGLOB },
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
  
  printf ("usage: %s [options] [VAR=VALUE...]\n", progname);
  printf ("options are:\n");
  printf (" [-]trimnl\n");
  printf (" [-]plaintext\n");
  printf (" -env\n");
  printf (" env sys|none|null\n");
  putchar ('\n');
  for (i = 0; bool_keytab[i].name; i++)
    printf (" [-]%s\n", bool_keytab[i].name);
  putchar ('\n');
  for (i = 0; string_keytab[i].name; i++)
    {
      printf (" -%s\n", string_keytab[i].name);
      printf (" %s ARG\n", string_keytab[i].name);
    }
  printf (" escape-word ARG\n");
  printf (" escape-quote ARG\n");
  putchar ('\n');
  for (i = 0; opt_keytab[i].name; i++)
    {
      printf (" [-]%s\n", opt_keytab[i].name);
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
      qbuf = mu_realloc (qbuf, qlen);
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

  size = i * 2 + 1;
  newenv = mu_calloc (size, sizeof (newenv[0]));

  for (i = j = 0; environ[i]; i++)
    {
      size_t len = strcspn (environ[i], "=");
      char *p = mu_alloc (len+1);
      memcpy (p, environ[i], len);
      p[len] = 0;
      newenv[j++] = p;
      p = mu_strdup (environ[i] + len + 1);
      newenv[j++] = p;
    }
  newenv[j] = NULL;
  return newenv;
}

static int
wsp_getvar (char **ret, const char *vptr, size_t vlen, void *data)
{
  char **base = data;
  int i;

  for (i = 0; base[i]; i++)
    {
      size_t l = strcspn (base[i], "=");
      if (l == vlen && memcmp (base[i], vptr, vlen) == 0)
	{
	  char *p = strdup (base[i] + vlen + 1);
	  if (p == NULL)
	    return MU_WRDSE_NOSPACE;
	  *ret = p;
	  return MU_WRDSE_OK;
	}
    }
  return MU_WRDSE_UNDEF;
}

static int
wsp_runcmd (char **ret, const char *str, size_t len, char **argv, void *closure)
{
  FILE *fp;
  char *cmd;
  int c, lastc;
  char *buffer = NULL;
  size_t bufsize = 0;
  size_t buflen = 0;
  
  cmd = malloc (len + 1);
  if (!cmd)
    return MU_WRDSE_NOSPACE;
  memcpy (cmd, str, len);
  cmd[len] = 0;

  fp = popen (cmd, "r");
  if (!fp)
    {
      ret = NULL;
      if (mu_asprintf (ret, "can't run %s: %s", cmd, mu_strerror (errno)))
	return MU_WRDSE_NOSPACE;
      else
	return MU_WRDSE_USERERR;
    }

  while ((c = fgetc (fp)) != EOF)
    {
      lastc = c;
      if (c == '\n')
	c = ' ';
      if (buflen == bufsize)
	{
	  char *p;
	  
	  if (bufsize == 0)
	    bufsize = 80;
	  else
	    bufsize *= 2;
	  p = realloc (buffer, bufsize);
	  if (!p)
	    {
	      free (buffer);
	      free (cmd);
	      return MU_WRDSE_NOSPACE;
	    }
	  buffer = p;
	}
      buffer[buflen++] = c;
    }

  if (buffer)
    {
      if (lastc == '\n')
	--buflen;
      buffer[buflen] = 0;
    }
  
  pclose (fp);
  free (cmd);

  *ret = buffer;
  return MU_WRDSE_OK;
}

enum env_type
  {
    env_none,
    env_null,
    env_sys
  };

struct mu_kwd env_keytab[] = {
  { "none",   env_none },
  { "null",   env_null },
  { "sys",    env_sys },
  { NULL }
};

static void
set_escape_string (mu_wordsplit_t *ws, int *wsflags, int q, const char *str)
{
  if (*str == ':')
    {
      while (*++str != ':')
	{
	  int f;
	  switch (*str)
	    {
	    case '+':
	      f = MU_WRDSO_BSKEEP;
	      break;

	    case '0':
	      f = MU_WRDSO_OESC;
	      break;

	    case 'x':
	      f = MU_WRDSO_XESC;
	      break;

	    default:
	      fprintf (stderr, "%s: invalid escape flag near %s\n",
		       progname, str);
	      abort ();
	    }
	  MU_WRDSO_ESC_SET (ws, q, f);
	}
      *wsflags |= MU_WRDSF_OPTIONS;
      ++str;
    }
  ws->ws_escape[q] = str;
}

int
main (int argc, char **argv)
{
  char buf[1024], *ptr, *saved_ptr;
  int i, offarg = 0;
  int trimnl_option = 0;
  int plaintext_option = 0;
  int wsflags = (MU_WRDSF_DEFFLAGS & ~MU_WRDSF_NOVAR) |
                 MU_WRDSF_ENOMEMABRT |
                 MU_WRDSF_SHOWERR;
  mu_wordsplit_t ws;
  int next_call = 0;
  char *fenvbase[128];
  size_t fenvidx = 0;
  size_t fenvmax = sizeof (fenvbase) / sizeof (fenvbase[0]);
  int use_env = env_sys;
  
  progname = argv[0];

  ws.ws_options = 0;
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

      if (strcmp (opt, "env") == 0)
	{
	  if (negate)
	    use_env = env_none;
	  else
	    {
	      i++;
	      if (i == argc)
		{
		  fprintf (stderr, "%s: missing argument for env\n",
			   progname);
		  exit (1);
		}

	      if (mu_kwd_xlat_name (env_keytab, argv[i], &use_env))
		{
		  fprintf (stderr, "%s: invalid argument for env\n",
			   progname);
		  exit (1);
		}
	    }
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
		  fprintf (stderr, "%s: missing argument for %s\n",
			   progname, opt);
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
		  set_escape_string (&ws, &wsflags, 0, argv[i]);
		  set_escape_string (&ws, &wsflags, 1, argv[i]);
		  break;
		}
	      
	      wsflags |= flag;
	    }
	  continue;
	}

      if (strcmp (opt, "escape-word") == 0
	  || strcmp (opt, "escape-quote") == 0)
	{
	  int q = opt[7] == 'q';
	  
	  i++;
	  if (i == argc)
	    {
	      fprintf (stderr, "%s: missing argument for %s\n",
		       progname, opt);
	      exit (1);
	    }
	  if (!(wsflags & MU_WRDSF_ESCAPE))
	    {
	      wsflags |= MU_WRDSF_ESCAPE;
	      ws.ws_escape[!q] = NULL;
	    }
	  set_escape_string (&ws, &wsflags, q, argv[i]);
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
		  fprintf (stderr, "%s: missing arguments for %s\n",
			   progname, opt);
		  exit (1);
		}
	      ws.ws_offs = strtoul (argv[i], &p, 10);
	      if (*p)
		{
		  fprintf (stderr, "%s: invalid number: %s\n",
			   progname, argv[i]);
		  exit (1);
		}

	      i++;
	      if (i + ws.ws_offs > argc)
		{
		  fprintf (stderr, "%s: not enough arguments for %s\n",
			   progname, opt);
		  exit (1);
		}
	      offarg = i;
	      i += ws.ws_offs - 1;
	      wsflags |= MU_WRDSF_DOOFFS;
	    }
	  continue;
	}

      if (mu_kwd_xlat_name (opt_keytab, opt, &flag) == 0)
	{
	  wsflags |= MU_WRDSF_OPTIONS;
	  if (negate)
	    ws.ws_options &= ~flag;
	  else
	    ws.ws_options |= flag;
	  continue;
	}
      
      if (strchr (opt, '='))
	{
	  if (fenvidx < fenvmax - 1)
	    {
	      fenvbase[fenvidx++] = opt;
	      continue;
	    }
	  else
	    {
	      fprintf (stderr, "%s: environment too big\n", progname);
	      exit (1);
	    }
	}
      
      fprintf (stderr, "%s: unrecognized argument: %s\n",
	       progname, opt);
      exit (1);
    }

  if (fenvidx)
    {
      fenvbase[fenvidx] = NULL;
      wsflags |= MU_WRDSF_GETVAR | MU_WRDSF_CLOSURE;
      ws.ws_getvar = wsp_getvar;
      ws.ws_closure = fenvbase;
    }

  switch (use_env)
    {
    case env_null:
      wsflags |= MU_WRDSF_ENV;
      ws.ws_env = NULL;
      break;

    case env_none:
      break;

    case env_sys:
      wsflags |= MU_WRDSF_ENV;
      if (wsflags & MU_WRDSF_ENV_KV)
	ws.ws_env = (const char **) make_env_kv ();
      else
	ws.ws_env = (const char **) environ;
      break;
    }
  
  if (!(wsflags & MU_WRDSF_NOCMD))
    ws.ws_command = wsp_runcmd;
  
  if (wsflags & MU_WRDSF_INCREMENTAL)
    trimnl_option = 1;
  
  next_call = 0;
  while ((ptr = fgets (buf, sizeof (buf), stdin)))
    {
      int rc;
      size_t i;
      
      if (trimnl_option)
	{
	  size_t len = strlen (ptr);
	  if (len && ptr[len-1] == '\n')
	    ptr[len-1] = 0;
	}
      
      if (wsflags & MU_WRDSF_INCREMENTAL)
	{
	  if (next_call)
	    {
	      if (*ptr == 0)
		ptr = NULL;
	      else
		free (saved_ptr);
	    }
	  else
	    next_call = 1;
	  if (ptr)
	    ptr = saved_ptr = mu_strdup (ptr);
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

      wsflags |= MU_WRDSF_REUSE | (ws.ws_flags & MU_WRDSF_ENV);
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
