/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#include "imap4d.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#define MATCH            (1 << 0)
#define RECURSE_MATCH    (1 << 1)
#define NOMATCH          (1 << 2)
#define NOSELECT         (1 << 3)
#define NOINFERIORS      (1 << 4)
#define NOSELECT_RECURSE (1 << 5)

/*
 *
 */

static void unquote (char **);
static int match (const char *, const char *);
static int imap_match (const char *, const char *);
static void list_file (const char *, const char *);
static char *expand (const char *);
static void print_file (const char *, const char *);
static void print_dir (const char *, const char *);

int
imap4d_list (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *reference;
  char *wildcard;

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  reference = util_getword (arg, &sp);
  wildcard = util_getword (NULL, &sp);
  if (!reference || !wildcard)
    return util_finish (command, RESP_BAD, "Too few arguments");

  unquote (&reference);
  unquote (&wildcard);
  if (*wildcard == '\0')
    {
      /* FIXME: How do we know the hierarchy delimeter.  */
      util_out (RESP_NONE, "LIST (\\NoSelect) \"/\" \"%s\"",
		(*reference == '\0' || *reference != '/') ? "" : "/");
    }
  else
    {
      char *p;

      if (*reference == '\0')
	reference = homedir;

      p = expand (reference);
      if (chdir (p) == 0)
	{
	  list_file (reference, wildcard);
	  chdir (homedir);
	}
      free (p);
    }
  return util_finish (command, RESP_OK, "Completed");
}

/* expand the '~'  */
static char *
expand (const char *ref)
{
  /* FIXME: note done.  */
  return strdup (ref);
}

/* Remove the surrounding double quotes.  */
static void
unquote (char **ptr)
{
  char *s = *ptr;
  if (*s == '"')
    {
      char *p = ++s;
      while (*p && *p != '"')
	p++;
      if (*p == '"')
	*p = '\0';
    }
  *ptr = s;
}

static void
print_file (const char *ref, const char *file)
{
  if (strpbrk (file, "\"{}"))
    {
      util_out (RESP_NONE, "LIST (\\NoInferior \\UnMarked) \"/\" {%d}",
		strlen (ref) + strlen ((*ref) ? "/" : "") + strlen (file));
      util_send ("%s%s%s\r\n", ref, (*ref) ? "/" : "", file);
    }
  else
    util_out (RESP_NONE, "LIST (\\NoInferior \\UnMarked) \"/\" %s%s%s",
	      ref, (*ref) ? "/" : "", file);
}

static void
print_dir (const char *ref, const char *file)
{
  if (strpbrk (file, "\"{}"))
    {
      util_out (RESP_NONE, "LIST (\\NoSelect) \"/\" {%d}",
		strlen (ref) + strlen ((*ref) ? "/" : "") + strlen (file));
      util_send ("%s%s%s\r\n", ref, (*ref) ? "/" : "", file);
    }
  else
    util_out (RESP_NONE, "LIST (\\NoSelect) \"/\" %s%s%s",
	      ref, (*ref) ? "/" : "", file);
}

/* Recusively calling the files.  */
static void
list_file (const char *ref, const char *pattern)
{
  DIR *dirp;
  struct dirent *dp;

  dirp = opendir (".");
  if (dirp == NULL)
    return;

  while ((dp = readdir (dirp)) != NULL)
    {
      int status = match (dp->d_name, pattern);
      if (status & (MATCH | RECURSE_MATCH))
	{
	  if (status & NOSELECT)
	    {
	      print_dir (ref, dp->d_name);
	      if (status & RECURSE_MATCH)
		{
		  if (chdir (dp->d_name) == 0)
		    {
		      char *buffer = NULL;
		      asprintf (&buffer, "%s%s%s", ref,
				(*ref) ? "/" : "", dp->d_name);
		      list_file (buffer, pattern);
		      free (buffer);
		      chdir ("..");
		    }
		}
	    }
	  else if (status & NOINFERIORS)
	    {
	      print_file (ref, dp->d_name);
	    }
	}
    }
}

static int
match (const char *entry, const char *pattern)
{
  struct stat stats;
  int status = imap_match (entry, pattern);
  if (status & MATCH || status || RECURSE_MATCH)
    {
      if (stat (entry, &stats) == 0)
	status |=  (S_ISREG (stats.st_mode)) ? NOINFERIORS : NOSELECT;
    }
  return status;
}

/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, nonzero if not.  */
static int
imap_match (const char *string, const char *pattern)
{
  const char *p = pattern, *n = string;
  char c;

  for (;(c = *p++) != '\0'; n++)
    {
      switch (c)
	{
	case '%':
	  if (*p == '\0')
	    return (*n == '/') ? NOMATCH : MATCH;
	  for (; *n != '\0'; ++n)
	    if (match (n, p) == MATCH)
	      return MATCH;
	  break;

	case '*':
	  if (*p == '\0')
	    return RECURSE_MATCH;
	  for (; *n != '\0'; ++n)
	    {
	      int status = match (n, p);
	      if (status == MATCH || status == RECURSE_MATCH)
		return status;
	    }
	  break;

	default:
	  if (c != *n)
	    return NOMATCH;
	}
    }

  if (*n == '\0')
    return MATCH;

  return NOMATCH;

}

