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
#include <dirent.h>
#include <pwd.h>

#define NOMATCH          (0)
#define MATCH            (1 << 0)
#define RECURSE_MATCH    (1 << 1)
#define NOSELECT         (1 << 2)
#define NOINFERIORS      (1 << 3)
#define NOSELECT_RECURSE (1 << 4)

/*
  1- IMAP4 insists: the reference argument that is include in the
  interpreted form SHOULD prefix the interpreted form.  It SHOULD
  also be in the same form as the reference name argument.  This
  rule permits the client to determine if the returned mailbox name
  is in the context of the reference argument, or if something about
  the mailbox argument overrode the reference argument.
  ex:
  Reference         Mailbox         -->  Interpretation
  ~smith/Mail        foo.*          -->  ~smith/Mail/foo.*
  archive            %              --> archive/%
  #news              comp.mail.*     --> #news.comp.mail.*
  ~smith/Mail        /usr/doc/foo   --> /usr/doc/foo
  archive            ~fred/Mail     --> ~fred/Mail/ *

  2- The character "*" is a wildcard, and matches zero or more characters
  at this position.  The charcater "%" is similar to "*",
  but it does not match ahierarchy delimiter.  */

static int  match __P ((const char *, const char *, const char *));
static int  imap_match __P ((const char *, const char *, const char *));
static void list_file __P ((const char *, const char *, const char *, const char *));
static void print_file __P ((const char *, const char *, const char *));
static void print_dir __P ((const char *, const char *, const char *));

int
imap4d_list (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *ref;
  char *wcard;
  const char *delim = "/";

  if (! (command->states & state))
    return util_finish (command, RESP_BAD, "Wrong state");

  ref = util_getword (arg, &sp);
  wcard = util_getword (NULL, &sp);
  if (!ref || !wcard)
    return util_finish (command, RESP_BAD, "Too few arguments");

  /* Remove the double quotes.  */
  util_unquote (&ref);
  util_unquote (&wcard);

  /* If wildcard is empty, it is a special case: we have to
     return the hierarchy.  */
  if (*wcard == '\0')
    {
      util_out (RESP_NONE, "LIST (\\NoSelect) \"%s\" \"%s\"", delim,
		(*ref) ? delim : "");
    }
  /* There is only one mailbox in the "INBOX" hierarchy ... INBOX.  */
  else if (strcasecmp (ref, "INBOX") == 0)
    {
      util_out (RESP_NONE, "LIST (\\NoInferiors) NIL INBOX");
    }
  else
    {
      char *cwd;
      char *dir;
      switch (*wcard)
	{
	  /* Absolute Path in wcard, dump the old ref.  */
	case '/':
	  {
	    ref = calloc (2, 1);
	    ref[0] = *wcard;
	    wcard++;
	  }
	  break;

	  /* Absolute Path, but take care of things like ~guest/Mail,
	     ref becomes ref = ~guest.  */
	case '~':
	  {
	    char *s = strchr (wcard, '/');
	    if (s)
	      {
		ref = calloc (s - wcard + 1, 1);
		memcpy (ref, wcard, s - wcard);
		ref [s - wcard] = '\0';
		wcard = s + 1;
	      }
	    else
	      {
		ref = strdup (wcard);
		wcard += strlen (wcard);
	      }
	  }
	  break;

	default:
	  ref = strdup (ref);
	}

      /* Move any directory not containing a wildcard into the reference
	 So (ref = ~guest, wcard = Mail/folder1/%.vf) -->
	 (ref = ~guest/Mail/folder1, wcard = %.vf).  */
      for (; (dir = strpbrk (wcard, "/%*")); wcard = dir)
	{
	  if (*dir == '/')
	    {
	      *dir = '\0';
	      ref = realloc (ref, strlen (ref) + 1 + (dir - wcard) + 1);
	      if (*ref && ref[strlen (ref) - 1] != '/')
		strcat (ref, "/");
	      strcat (ref, wcard);
	      dir++;
	    }
	  else
	    break;
	}

      /* Allocates.  */
      cwd = namespace_checkfullpath (ref, wcard, delim);
      if (!cwd)
	{
	  free (ref);
	  return util_finish (command, RESP_NO,
			      "The requested item could not be found.");
	}

      /* If wcard match inbox return it too, part of the list.  */
      if (!*ref && (match ("INBOX", wcard, delim)
		    || match ("inbox", wcard, delim)))
	util_out (RESP_NONE, "LIST (\\NoInferiors) NIL INBOX");
      
      if (chdir (cwd) == 0)
	{
	  list_file (cwd, ref, (dir) ? dir : "", delim);
	  chdir (homedir);
	}
      free (cwd);
      free (ref);
    }

  return util_finish (command, RESP_OK, "Completed");
}

/* Recusively calling the files.  */
static void
list_file (const char *cwd, const char *ref, const char *pattern,
	   const char *delim)
{
  DIR *dirp;
  struct dirent *dp;
  char *next;

  /* Shortcut no wildcards.  */
  if (*pattern == '\0' || !strpbrk (pattern, "%*"))
    {
      /* Equivalent to stat().  */
      int status;
      if (*pattern == '\0')
	status = match (cwd, cwd, delim);
      else
	status = match (pattern, pattern, delim);
      if (status & NOSELECT)
	print_dir (ref, pattern, delim);
      else if (status & NOINFERIORS)
	print_file (ref, pattern, delim);
      return ;
    }

  dirp = opendir (".");
  if (dirp == NULL)
    return;

  next = strchr (pattern, delim[0]);
  if (next)
    *next++ = '\0';
  while ((dp = readdir (dirp)) != NULL)
    {
      /* Skip "", ".", and "..".  "" is returned by at least one buggy
	 implementation: Solaris 2.4 readdir on NFS filesystems.  */
      char const *entry = dp->d_name;
      if (entry[entry[0] != '.' ? 0 : entry[1] != '.' ? 1 : 2] != '\0')
	{
	  int status = match (entry, pattern, delim);
	  if (status)
	    {
	      if (status & NOSELECT)
		{
		  if (next || status & RECURSE_MATCH)
		    {
		      if (!next)
			print_dir (ref, entry, delim);
		      if (chdir (entry) == 0)
			{
			  char *rf;
			  char *cd;
			  rf = calloc (strlen (ref) + strlen (delim) +
				       strlen (entry) + 1, 1);
			  sprintf (rf, "%s%s%s", ref, delim, entry);
			  cd = calloc (strlen (cwd) + strlen (delim) +
				      strlen (entry) + 1, 1);
			  sprintf (cd, "%s%s%s", cwd, delim, entry);
			  list_file (cd, rf, (next) ? next : pattern, delim);
			  free (rf);
			  free (cd);
			  chdir (cwd);
			}
		    }
		  else
		    print_dir (ref, entry, delim);
		}
	      else if (status & NOINFERIORS)
		{
		  print_file (ref, entry, delim);
		}
	    }
	}
    }
  closedir (dirp);
}

/* Make sure that the file name does not contain any undesirable
   chars like "{}. If yes send it as a literal string.  */
static void
print_file (const char *ref, const char *file, const char *delim)
{
  if (strpbrk (file, "\"{}"))
    {
      util_out (RESP_NONE, "LIST (\\NoInferiors) \"%s\" {%d}", delim,
		strlen (ref) + strlen ((*ref) ? delim : "") + strlen (file));
      util_send ("%s%s%s\r\n", ref, (*ref) ? delim : "", file);
    }
  else
    util_out (RESP_NONE, "LIST (\\NoInferiors) \"%s\" %s%s%s", delim,
	      ref, (*ref) ? delim : "", file);
}

/* Make sure that the file name does not contain any undesirable
   chars like "{}. If yes send it as a literal string.  */
static void
print_dir (const char *ref, const char *file, const char *delim)
{
  if (strpbrk (file, "\"{}"))
    {
      util_out (RESP_NONE, "LIST (\\NoSelect) \"%s\" {%d}", delim,
		strlen (ref) + strlen ((*ref) ? delim : "") + strlen (file));
      util_send ("%s%s%s\r\n", ref, (*ref) ? delim : "", file);
    }
  else
    util_out (RESP_NONE, "LIST (\\NoSelect) \"%s\" %s%s%s", delim,
	      ref, (*ref) ? delim : "", file);
}

/* Calls the imap_matcher if a match found out the attribute. */
static int
match (const char *entry, const char *pattern, const char *delim)
{
  struct stat stats;
  int status = imap_match (entry, pattern, delim);
  if (status)
    {
      if (stat (entry, &stats) == 0)
	status |=  (S_ISREG (stats.st_mode)) ? NOINFERIORS : NOSELECT;
    }
  return status;
}

/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, nonzero if not.  */
static int
imap_match (const char *string, const char *pattern, const char *delim)
{
  const char *p = pattern, *n = string;
  char c;

  for (;(c = *p++) != '\0' && *n; n++)
    {
      switch (c)
	{
	case '%':
	  if (*p == '\0')
	    {
	      /* Matches everything except '/' */
	      for (; *n && *n != delim[0]; n++)
		;
	      return (*n == '/') ? RECURSE_MATCH : MATCH;
	    }
	  else
	    for (; *n != '\0'; ++n)
	      if (imap_match (n, p, delim) == MATCH)
		return MATCH;
	  break;

	case '*':
	  if (*p == '\0')
	    return RECURSE_MATCH;
	  for (; *n != '\0'; ++n)
	    {
	      int status = imap_match (n, p, delim);
	      if (status == MATCH || status == RECURSE_MATCH)
		return status;
	    }
	  break;

	default:
	  if (c != *n)
	    return NOMATCH;
	}
    }

  if (!c && !*n)
    return MATCH;

  return NOMATCH;

}
