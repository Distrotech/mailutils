/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <folder0.h>
#include <registrar0.h>


static int fmbox_init (folder_t);

struct folder_entry _fmbox_entry =
{
  url_path_init, fmbox_init
};

/* We export two functions: url parsing and the initialisation of
   the mailbox, via the register entry/record.  */
folder_entry_t fmbox_entry = &_fmbox_entry;

static void fmbox_destroy (folder_t);
static int fmbox_open (folder_t, int);
static int fmbox_close (folder_t);
static int fmbox_delete (folder_t, const char *);
static int fmbox_list (folder_t, const char *,
		       struct folder_list ***, size_t *);

static int fmbox_list0 (const char *, struct folder_list ***, size_t *, int);

struct _fmbox
{
  char *dirname;
};
typedef struct _fmbox *fmbox_t;


static int
fmbox_init (folder_t folder)
{
  fmbox_t dfolder;
  //char *dirname;
  size_t name_len = 0;

  dfolder = folder->data = calloc (1, sizeof (dfolder));
  if (dfolder == NULL)
    return ENOMEM;

  url_get_path (folder->url, NULL, 0, &name_len);
  dfolder->dirname = calloc (name_len + 1, sizeof (char));
  if (dfolder->dirname == NULL)
    {
      free (dfolder);
      folder->data = NULL;
      return ENOMEM;
    }
  url_get_path (folder->url, dfolder->dirname, name_len + 1, NULL);

  folder->_init = fmbox_init;
  folder->_destroy = fmbox_destroy;

  folder->_open = fmbox_open;
  folder->_close = fmbox_close;

  folder->_list = fmbox_list;
  folder->_delete_mailbox = fmbox_delete;

  return 0;
}

void
fmbox_destroy (folder_t folder)
{
  if (folder->data)
    {
      fmbox_t fmbox = folder->data;
      if (fmbox->dirname)
	free (fmbox->dirname);
      free (folder->data);
      folder->data = NULL;
    }
}

static int
fmbox_open (folder_t folder, int flags)
{
  (void)(folder);
  (void)(flags);
  return 0;
}

static int
fmbox_close (folder_t folder)
{
  (void)(folder);
  return 0;
}

static int
fmbox_delete (folder_t folder, const char *dirname)
{
  (void)(folder);
  (void)dirname;
  return 0;
}

static int
fmbox_list (folder_t folder, const char *pattern,
	    struct folder_list ***pflist, size_t *pnum)
{
  fmbox_t fmbox = folder->data;
  struct folder_list **list = NULL;
  char *dirname = NULL;
  int status;
  size_t num = 0;

  if (pattern == NULL)
    {
      dirname = strdup (fmbox->dirname);
    }
  else if (pattern[0] != '/')
    {
      size_t len = strlen (pattern);
      dirname = calloc (strlen (fmbox->dirname) + len + 2, sizeof (char));
      if (dirname)
	  sprintf (dirname, "%s/%s", fmbox->dirname, pattern);
    }
  else
    dirname = strdup (pattern);

  if (dirname == NULL)
    return errno;

  status = fmbox_list0 (dirname, &list, &num, (strchr (dirname, '*') != NULL));
  if (status == 0)
    {
      if (pflist)
	*pflist = list;
      if (pnum)
	*pnum = num;
    }
  return status;
}

static int
fmbox_list0 (const char *dirname, struct folder_list ***pflist,
	     size_t *pnum, int recurse)
{
  DIR *dp;
  struct dirent *rp;
  struct stat stbuf;
  struct folder_list **flist = *pflist;
  char buf[512];
  dp = opendir (dirname);
  if (dp == NULL)
    {
      return errno;
    }
  while ((rp = readdir (dp)) != NULL)
    {
      if (rp->d_name[0] == '.')
	continue;
      flist = realloc (flist, (*pnum + 1) * sizeof (*flist));
      flist[*pnum] = calloc (1, sizeof (**flist));
      sprintf (buf, "%s/%s", dirname, rp->d_name);
      if (stat (buf, &stbuf) == 0)
	{
	  int isdir = S_ISDIR(stbuf.st_mode);
	  if (isdir)
	    flist[*pnum]->attribute |= MU_FOLDER_ATTRIBUTE_NOSELECT;
	  flist[*pnum]->name = strdup (buf);
	  (*pnum)++;
	  if (isdir && recurse)
	    {
		fmbox_list0 (buf, &flist, pnum, recurse);
	    }
	}
      else
	(*pnum)++;
    }
  closedir (dp);
  *pflist = flist;
  return 0;
}
