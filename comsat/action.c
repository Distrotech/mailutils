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

#include "comsat.h"
#define obstack_chunk_alloc malloc
#define obstack_chunk_free free
#include <obstack.h>

/* This module implements user-configurable actions for comsat. The
   actions are kept in file .biffrc in the user's home directory and
   are executed in sequence. Possible actions:

   beep              --  Produce audible signal
   echo STRING       --  Output STRING to the user's tty
   exec PROG ARGS... --  Execute given program (absolute pathname
                         required) 

   Following metacharacters are accepted in strings:

   $u        --  Expands to username
   $h        --  Expands to hostname
   $H{name}  --  Expands to value of message header `name'
   $B(C,L)   --  Expands to message body. C and L give maximum
                 number of characters and line in the expansion.
		 When omitted, they default to 400, 5. */

int
act_getline (FILE *fp, char **sptr, size_t *size)
{
  char buf[256];
  int cont = 1;
  size_t used = 0;
  
  while (cont && fgets (buf, sizeof buf, fp))
    {
      int len = strlen (buf);
      if (buf[len-1] == '\n')
	{
	  buf[--len] = 0;
	  if (buf[len-1] == '\\')
	    {
	      buf[--len] = 0;
	      cont = 1;
	    }
	  else
	    cont = 0;
	}
      else
	cont = 1;

      if (len + used + 1 > *size)
	{
	  *sptr = realloc (*sptr, len + used + 1);
	  if (!*sptr)
	    return -1;
	  *size = len + used + 1;
	}
      memcpy (*sptr + used, buf, len);
      used += len;
    }    

  if (*sptr)
    (*sptr)[used] = 0;
  
  return used;
}

/* Convert second character of a backslash sequence to its ASCII
   value: */
int
backslash(int c)
{
  static char transtab[] = "a\ab\bf\fn\nr\rt\t";
  char *p;
  
  for (p = transtab; *p; p += 2)
    {
      if (*p == c)
	return p[1];
    }
  return c;
}

int
expand_escape (char **pp, message_t msg, char *cr, struct obstack *stk)
{
  char *p = *pp;
  char *start, *sval, *namep;
  int len;
  header_t hdr;
  body_t body;
  stream_t stream;
  int rc = 1;
  size_t size, lncount;
  
  switch (*++p) /* skip past $ */
    {
    case 'u':
      len = strlen (username);
      obstack_grow (stk, username, len);
      *pp = p;
      rc = 0;
      break;
    case 'h':
      len = strlen (hostname);
      obstack_grow (stk, hostname, len);
      *pp = p;
      rc = 0;
      break;
    case 'H':
      /* Header field */
      if (*++p != '{')
	break;
      start = ++p;
      p = strchr (p, '}');
      if (!p)
	break;
      len = p - start;
      if (len == 0
	  || (namep = malloc (len + 1)) == NULL)
	break;
      memcpy (namep, start, len);
      namep[len] = 0;
      if (message_get_header (msg, &hdr) == 0
	  && header_aget_value (hdr, namep, &sval) == 0)
	{
	  len = strlen (sval);
	  obstack_grow (stk, sval, len);
	}
      free (namep);
      *pp = p;
      rc = 0;
      break;
    case 'B':
      /* Message body */
      if (*++p == '(')
	{
	  size = strtoul (p + 1, &p, 0);
	  if (*p == ',')
	    lncount = strtoul (p + 1, &p, 0);
	  if (*p != ')')
	    break;
	}
      if (size == 0)
	size = 400;
      if (lncount == 0)
	lncount = maxlines;
      if (message_get_body (msg, &body) == 0
	  && body_get_stream (body, &stream) == 0)
	{
	  size_t nread;
	  char *buf = malloc (size+1);
	  
	  if (!buf)
	    break;
	  if (stream_read (stream, buf, size, 0, &nread) == 0)
	    {
	      char *q;

	      buf[nread] = 0;
	      q = buf;
	      size = 0;
	      while (lncount--)
		{
		  char *s = strchr (q, '\n');
		  if (!s)
		    break;
		  size = s - q;
		  obstack_grow (stk, q, size);
		  obstack_grow (stk, cr, strlen (cr));
		  q = s + 1;
		}
	    }
	  free (buf);
	}
      *pp = p;
      rc = 0;
    }
  return rc;
}

char *
expand_line (char *str, char *cr, message_t msg)
{
  char *p;
  int i, c, len;
  size_t size;
  struct obstack stk;
  
  if (!*str)
    return NULL;
  obstack_init (&stk);
  for (p = str; *p; p++)
    {
      switch (*p)
	{
	case '\n':
	  len = strlen (cr);
	  obstack_grow (&stk, cr, len);
	  break;
	case '\\':
	  p++;
	  switch (*p)
	    {
	    case 0:
	      obstack_1grow (&stk, c);
	      break;
	    case 'n':
	      len = strlen (cr);
 	      obstack_grow (&stk, cr, len);
	      break;
	    default:
	      c = backslash (*p);
	      obstack_1grow (&stk, c);
	    }
	  break;
	case '$':
	  if (expand_escape (&p, msg, cr, &stk) == 0)
	    break;
	  /*FALLTHRU*/
	default:
	  obstack_1grow (&stk, *p);
	}
    }
  obstack_1grow (&stk, 0);
  str = strdup (obstack_finish (&stk));
  obstack_free (&stk, NULL);
  return str;
}

char *default_action =
"Mail to \a$u@$h\a\n"
"---\n"
"From: $H{from}\n"
"Subject: $H{Subject}\n"
"---\n"
"$B(,5)\n"
"---\n";

/* Take care to clear eighth bit, so we won't upset some stupid terminals */
#define LB(c) ((c)&0x7f)
  
void
action_beep (FILE *tty)
{
  fprintf (tty, "\a\a");
}

void
action_echo (FILE *tty, char *str)
{
  char *p;
  
  if (!str)
    return;
  for (p = str; *p; p++)
    *p = LB (*p);
  fprintf (tty, "%s", str);
}

void
action_exec (FILE *tty, int line, int argc, char **argv)
{
  pid_t pid;
  struct stat stb;

  if (argc == 0)
    {
      syslog (LOG_ERR, "%s:.biffrc:%d: No arguments for exec", username, line);
      return;
    }

  if (argv[0][0] != '/')
    {
      syslog (LOG_ERR, "%s:.biffrc:%d: Not an absolute pathname",
	      username, line);
      return;
    }

  if (stat (argv[0], &stb))
    {
      syslog (LOG_ERR, "%s:.biffrc:%d: can't stat %s: %s",
	      username, line, argv[0], strerror (errno));
      return;
    }

  if (stb.st_mode & (S_ISUID|S_ISGID))
    {
      syslog (LOG_ERR, "%s:.biffrc:%d: won't execute set[ug]id programs",
	      username, line);
      return;
    }

  pid = fork ();
  if (pid == 0)
    {
      close (1);
      close (2);
      dup2 (fileno (tty), 1);
      dup2 (fileno (tty), 2);
      fclose (tty);
      execv (argv[0], argv);
      syslog (LOG_ERR, "can't execute %s: %s", argv[0], strerror (errno));
    }
}

FILE *
open_rc (char *filename, FILE *tty)
{
  struct stat stb;
  struct passwd *pw = getpwnam (username);

  /* To be on the safe side, we do not allow root to have his .biffrc */
  if (!allow_biffrc || pw->pw_uid == 0)
    return NULL;
  if (stat (filename, &stb) == 0)
    {
      if (stb.st_uid != pw->pw_uid)
	{
	  syslog (LOG_NOTICE, "%s's %s is not owned by %s",
		  username, filename, username);
	  return NULL;
	}
      if ((stb.st_mode & 0777) != 0600)
	{
	  fprintf (tty, "Warning: your .biffrc has wrong permissions\r\n");
	  syslog (LOG_NOTICE, "%s's %s has wrong permissions",
		  username, filename);
	  return NULL;
	}
    }
  return fopen (filename, "r");
}

void
run_user_action (FILE *tty, char *cr, message_t msg)
{
  FILE *fp;
  int nact = 0;
  char *stmt = NULL;
  size_t size = 0;

  fp = open_rc (BIFF_RC, tty);
  if (fp)
    {
      int line = 0;
      
      while (act_getline (fp, &stmt, &size))
	{
	  char *str;
	  int argc;
	  char **argv;

	  line++;
	  str = expand_line (stmt, cr, msg);
	  if (!str)
	    continue;
	  if (argcv_get (str, "", &argc, &argv)
	      || argc == 0
	      || argv[0][0] == '#')
	    {
	      free (str);
	      argcv_free (argc, argv);
	      continue;
	    }

	  if (strcmp (argv[0], "beep") == 0)
	    {
	      action_beep (tty);
	      nact++;
	    }
	  else if (strcmp (argv[0], "echo") == 0)
	    {
	      action_echo (tty, argv[1]);
	      nact++;
	    }
	  else if (strcmp (argv[0], "exec") == 0)
	    {
	      action_exec (tty, line, argc-1, argv+1);
	      nact++;
	    }
	  else
	    {
	      fprintf (tty, ".biffrc:%d: unknown keyword\r\n");
	      syslog (LOG_ERR, "%s:.biffrc:%d: unknown keyword %s",
		      username, line, argv[0]);
	      break;
	    }
	}
      fclose (fp);
    }
  
  if (nact == 0)
    {
      char *str = expand_line (default_action, cr, msg);
      action_echo (tty, str);
    }
}


