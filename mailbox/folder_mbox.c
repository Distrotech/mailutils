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
#include <glob.h>
#include <stdio.h>

#include <folder0.h>
#include <registrar0.h>

/* We export url parsing and the initialisation of
   the mailbox, via the register entry/record.  */

static struct _record _mbox_record =
{
  MU_MBOX_SCHEME,
  _url_mbox_init, /* Mailbox init.  */
  _mailbox_mbox_init, /* Mailbox init.  */
  NULL, /* Mailer init.  */
  _folder_mbox_init, /* Folder init.  */
  NULL, /* No need for an back pointer.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
record_t mbox_record = &_mbox_record;

static struct _record _file_record =
{
  MU_FILE_SCHEME,
  _url_file_init,     /* Mailbox init.  */
  _mailbox_file_init, /* Mailbox init.  */
  NULL,               /* Mailer init.  */
  _folder_mbox_init,  /* Folder init.  */
  NULL, /* No need for an owner.  */
  NULL, /* _is_scheme method.  */
  NULL, /* _get_url method.  */
  NULL, /* _get_mailbox method.  */
  NULL, /* _get_mailer method.  */
  NULL  /* _get_folder method.  */
};
record_t file_record = &_file_record;

static struct _record _path_record =
{
  MU_PATH_SCHEME,
  _url_path_init,     /* Mailbox init.  */
  _mailbox_file_init, /* Mailbox init.  */
  NULL,               /* Mailer init.  */
  _folder_mbox_init,  /* Folder init.  */
  NULL, /* No need for an owner.  */
  NULL, /* is_scheme method.  */
  NULL, /* get_url method.  */
  NULL, /* get_mailbox method.  */
  NULL, /* get_mailer method.  */
  NULL  /* get_folder method.  */
};
record_t path_record = &_path_record;

/* lsub/subscribe/unsubscribe are not needed.  */
static void folder_mbox_destroy (folder_t);
static int folder_mbox_open (folder_t, int);
static int folder_mbox_close (folder_t);
static int folder_mbox_delete (folder_t, const char *);
static int folder_mbox_rename (folder_t , const char *, const char *);
static int folder_mbox_list (folder_t, const char *,
			     struct folder_list ***, size_t *);

static char *get_pathname (const char *, const char *);

struct _fmbox
{
  char *dirname;
};
typedef struct _fmbox *fmbox_t;


int
_folder_mbox_init (folder_t folder)
{
  fmbox_t dfolder;
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

  folder->_destroy = folder_mbox_destroy;

  folder->_open = folder_mbox_open;
  folder->_close = folder_mbox_close;

  folder->_list = folder_mbox_list;
  folder->_delete = folder_mbox_delete;
  folder->_rename = folder_mbox_rename;

  return 0;
}

void
folder_mbox_destroy (folder_t folder)
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

/* Noop. */
static int
folder_mbox_open (folder_t folder, int flags)
{
  (void)(folder);

  (void)(flags);
  return 0;
}

/*  Noop.  */
static int
folder_mbox_close (folder_t folder)
{
  (void)(folder);
  return 0;
}

static int
folder_mbox_delete (folder_t folder, const char *filename)
{
  fmbox_t fmbox = folder->data;
  if (filename)
    {
      int status = 0;
      char *pathname = get_pathname (fmbox->dirname, filename);
      if (pathname)
	{
	  if (remove (pathname) != 0)
	    status = errno;
	  free (pathname);
	}
      else
	status = ENOMEM;
      return status;
    }
  return EINVAL;
}

static int
folder_mbox_rename (folder_t folder, const char *oldpath, const char *newpath)
{
  fmbox_t fmbox = folder->data;
  if (oldpath && newpath)
    {
      int status = 0;
      char *pathold = get_pathname (fmbox->dirname, oldpath);
      if (pathold)
	{
	  char *pathnew = get_pathname (fmbox->dirname, newpath);
	  if (pathnew)
	    {
	      if (rename (pathold, pathnew) != 0)
		status = errno;
	      free (pathnew);
	    }
	  else
	    status = ENOMEM;
	  free (pathold);
	}
      else
	status = ENOMEM;
      return status;
    }
  return EINVAL;
}

/* The listing is not recursif and we use glob() some expansion for us.
   Unfortunately glov() does not expand the '~'.  We also return
   The full pathname so it can be use to create other folders.  */
static int
folder_mbox_list (folder_t folder, const char *pattern,
	    struct folder_list ***pflist, size_t *pnum)
{
  fmbox_t fmbox = folder->data;
  char *pathname = NULL;
  int status;
  size_t num = 0;
  glob_t gl;

  pathname = get_pathname (fmbox->dirname, pattern);
  if (pathname)
    {
      memset(&gl, 0, sizeof(gl));
      status = glob (pathname, 0, NULL, &gl);
      free (pathname);
      num = gl.gl_pathc;
    }
  else
    status = ENOMEM;

  /* Build the folder list from glob.  */
  if (status == 0)
    {
      if (pflist)
	{
	  struct folder_list **flist;
	  flist = calloc (num, sizeof (*flist));
	  if (flist)
	    {
	      size_t i;
	      struct stat stbuf;
	      for (i = 0; i < num; i++)
		{
		  flist[i] = calloc (1, sizeof (**flist));
		  if (flist[i] == NULL
		      || (flist[i]->name = strdup (gl.gl_pathv[i])) == NULL)
		    {
		      num = i;
		      break;
		    }
		  if (stat (gl.gl_pathv[i], &stbuf) == 0)
		    {
		      if (S_ISDIR(stbuf.st_mode))
			flist[i]->type = MU_FOLDER_ATTRIBUTE_DIRECTORY;
		      if (S_ISREG(stbuf.st_mode))
			flist[i]->type = MU_FOLDER_ATTRIBUTE_FILE;
		    }
		  flist[i]->separator = '/';
		}
	    }
	  else
	    status = ENOMEM;
	  *pflist = flist;
	}
      globfree (&gl);
    }
  else
    {
      status = (status == GLOB_NOSPACE) ? ENOMEM :
	((status == GLOB_NOMATCH) ? ENOENT : EINVAL);
    }
  if (pnum)
    *pnum = num;
  return status;
}

static char *
get_pathname (const char *dirname, const char *basename)
{
  char *pathname = NULL;
  /* null basename gives dirname.  */
  if (basename == NULL)
    pathname = (dirname) ? strdup (dirname) : strdup (".");
  /* Absolute.  */
  else if (basename[0] == '/')
    pathname = strdup (basename);
  /* Relative.  */
  else
    {
      size_t len = strlen (basename);
      pathname = calloc (strlen (dirname) + len + 2, sizeof (char));
      if (pathname)
	sprintf (pathname, "%s/%s", dirname, basename);
    }
  return pathname;
}
