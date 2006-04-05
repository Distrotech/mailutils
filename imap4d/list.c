/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001, 2002, 2005, 2006  Free Software Foundation, Inc.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301 USA */

#include "imap4d.h"
#include <dirent.h>
#include <pwd.h>

#define NOMATCH          (0)
#define MATCH            (1 << 0)
#define RECURSE_MATCH    (1 << 1)
#define NOSELECT         (1 << 2)
#define NOINFERIORS      (1 << 3)
#define NOSELECT_RECURSE (1 << 4)

struct inode_list
{
  struct inode_list *next;
  ino_t inode;
  dev_t dev;
};

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

static int  match (const char *, const char *, const char *);
static void list_file (const char *, const char *, const char *, const char *, struct inode_list *);
static void print_file (const char *, const char *, const char *);
static void print_dir (const char *, const char *, const char *);

int
imap4d_list (struct imap4d_command *command, char *arg)
{
  char *sp = NULL;
  char *ref;
  char *wcard;
  const char *delim = "/";

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
	    dir = wcard;	  
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

      /* The special name INBOX is included in the output from LIST, if
	 INBOX is supported by this server for this user and if the
	 uppercase string "INBOX" matches the interpreted reference and
	 mailbox name arguments with wildcards as described above.  The
	 criteria for omitting INBOX is whether SELECT INBOX will return
	 failure; it is not relevant whether the user's real INBOX resides
	 on this or some other server. */

      if (!*ref && (match ("INBOX", wcard, delim)
		    || match ("inbox", wcard, delim)))
	util_out (RESP_NONE, "LIST (\\NoInferiors) NIL INBOX");
      
      if (chdir (cwd) == 0)
	{
	  struct stat st;
	  struct inode_list inode_rec;
	  
	  stat (cwd, &st);
	  inode_rec.next = NULL;
	  inode_rec.inode = st.st_ino;
	  inode_rec.dev   = st.st_dev;
	  list_file (cwd, ref, (dir) ? dir : wcard, delim, &inode_rec);
	  chdir (homedir);
	}
      free (cwd);
      free (ref);
    }

  return util_finish (command, RESP_OK, "Completed");
}

static int
inode_list_lookup (struct inode_list *list, struct stat *st)
{
  for (; list; list = list->next)
    if (list->inode == st->st_ino && list->dev == st->st_dev)
      return 1;
  return 0;
}

static char *
mkfullname (const char *dir, const char *name, const char *delim)
{
  char *p;
  int dlen = strlen (dir);

  if (dlen == 0)
    return strdup (name);
  
  if (dir[dlen-1] == delim[0])
    dlen--;

  p = malloc (dlen + 1 + strlen (name) + 1);
  if (p)
    {
      memcpy (p, dir, dlen);
      p[dlen] = '/';
      strcpy (p + dlen + 1, name);
    }
  return p;
}

/* Recusively calling the files.  */
static void
list_file (const char *cwd, const char *ref, const char *pattern,
	   const char *delim, struct inode_list *inode_list)
{
  DIR *dirp;
  struct dirent *dp;
  char *next;

  if (!cwd || !ref)
    return;
  
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
      if (entry[entry[0] != '.' ? 0 : entry[1] != '.' ? 1 : 2] != '\0' &&
	  !(!strcmp (entry, "INBOX") && !strcmp(cwd, homedir)))
	{
	  int status = match (entry, pattern, delim);
	  if (status)
	    {
	      if (status & NOSELECT)
		{
		  struct stat st;

		  if (stat (entry, &st))
		    {
		      mu_error (_("Cannot stat %s: %s"),
				entry, strerror (errno));
		      continue;
		    }

		  if (next || status & RECURSE_MATCH)
		    {
		      if (!next)
			print_dir (ref, entry, delim);

		      if (S_ISDIR (st.st_mode)
			  && inode_list_lookup (inode_list, &st) == 0)
			{
			  if (chdir (entry) == 0)
			    {
			      char *rf;
			      char *cd;
			      struct inode_list inode_rec;

			      inode_rec.inode = st.st_ino;
			      inode_rec.dev   = st.st_dev;
			      inode_rec.next = inode_list;
			      rf = mkfullname (ref, entry, delim);
			      cd = mkfullname (cwd, entry, delim);
			      list_file (cd, rf, (next) ? next : pattern,
					 delim, &inode_rec);
			      free (rf);
			      free (cd);
			      chdir (cwd);
			    }
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

static void
print_name (const char *ref, const char *file, const char *delim,
	     const char *attr)
{
  char *name = mkfullname (ref, file, delim);
  if (strpbrk (name, "\"{}"))
    {
      util_out (RESP_NONE, "LIST (%s) \"%s\" {%d}",
		attr, delim, strlen (name));
      util_send ("%s\r\n", name);
    }
  else if (is_atom (name))
    util_out (RESP_NONE, "LIST (%s) \"%s\" %s", attr, delim, name);
  else
    util_out (RESP_NONE, "LIST (%s) \"%s\" \"%s\"", attr, delim, name);
  free (name);
}

static void
print_file (const char *ref, const char *file, const char *delim)
{
  print_name (ref, file, delim, "\\NoInferiors");
}

static void
print_dir (const char *ref, const char *file, const char *delim)
{
  print_name (ref, file, delim, "\\NoSelect");
}

/* Calls the imap_matcher if a match found out the attribute. */
static int
match (const char *entry, const char *pattern, const char *delim)
{
  struct stat stats;
  int status = util_wcard_match (entry, pattern, delim);

  switch (status)
    {
    case WCARD_RECURSE_MATCH:
      status = RECURSE_MATCH;
      break;
    case WCARD_MATCH:
      status = MATCH;
      break;
    case WCARD_NOMATCH:
      status = NOMATCH;
    }
  
  if (status)
    {
      if (stat (entry, &stats) == 0)
	status |=  (S_ISREG (stats.st_mode)) ? NOINFERIORS : NOSELECT;
    }
  return status;
}

