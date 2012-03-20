/* File safety checks.
   Copyright (C) 1999-2001, 2003-2006, 2010-2012 Free Software
   Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/types.h>
#include <mailutils/errno.h>
#include <mailutils/mu_auth.h>
#include <mailutils/list.h>
#include <mailutils/util.h>

/* Functions for checking mode of a file and the directory it resides in.
   Each of these checks certain bits and returns 0 if they are OK
   and non-0 otherwise. */

struct file_check_buffer
{
  struct stat filst;
  struct stat dirst;
  int cdir;
};

static int
_check_linkwrdir (struct file_check_buffer *fb)
{
  return ((((fb->filst.st_mode & S_IFMT) == S_IFLNK) ||
	   fb->filst.st_nlink > 1) &&
	  (fb->dirst.st_mode & (S_IWGRP | S_IWOTH)));
}

static int
_check_gwrdir (struct file_check_buffer *fb)
{
  return fb->dirst.st_mode & S_IWGRP;
}

static int
_check_awrdir (struct file_check_buffer *fb)
{
  return fb->dirst.st_mode & S_IWOTH;
}

/* The table of permission checkers below has this type: */
struct safety_checker
{
  char *name;            /* Symbolic name */
  int flag;              /* MU_FILE_SAFETY_ flag that enables this entry */
  int err;               /* Corresponding error code */
  int mode;              /* Corresponding file mode bit */
  int cdir;              /* True if the function needs dirst member */
  int (*fun) (struct file_check_buffer *fb); /* Checker function */
};

static struct safety_checker file_safety_check_tab[] = {
  { "grdfil", MU_FILE_SAFETY_GROUP_READABLE, MU_ERR_PERM_GROUP_READABLE,
    S_IRGRP },
  { "ardfil", MU_FILE_SAFETY_WORLD_READABLE, MU_ERR_PERM_WORLD_READABLE,
    S_IROTH },
  { "gwrfil", MU_FILE_SAFETY_GROUP_WRITABLE, MU_ERR_PERM_GROUP_WRITABLE,
    S_IWGRP },
  { "awrfil", MU_FILE_SAFETY_WORLD_WRITABLE, MU_ERR_PERM_WORLD_WRITABLE,
    S_IWOTH },
  { "linkwrdir", MU_FILE_SAFETY_LINKED_WRDIR,   MU_ERR_PERM_LINKED_WRDIR,
    0, 1, _check_linkwrdir },
  { "gwrdir", MU_FILE_SAFETY_DIR_IWGRP,      MU_ERR_PERM_DIR_IWGRP,
    0, 1, _check_gwrdir },
  { "awrdir", MU_FILE_SAFETY_DIR_IWOTH,      MU_ERR_PERM_DIR_IWOTH,
    0, 1, _check_awrdir },
  { 0 }
};

struct file_id
{
  dev_t dev;
  ino_t inode;
};

static int
file_id_cmp (const void *item, const void *data)
{
  const struct file_id *a = item;
  const struct file_id *b = data;

  if (a->dev != b->dev)
    return 1;
  if (a->inode != b->inode)
    return 1;
  return 0;
}

static int
file_id_lookup (mu_list_t idlist, dev_t dev, ino_t ino)
{
  struct file_id id;

  id.dev = dev;
  id.inode = ino;
  return mu_list_locate (idlist, &id, NULL);
}

static int
file_id_remember (mu_list_t idlist, dev_t dev, ino_t ino)
{
  struct file_id *id = malloc (sizeof (*id));
  if (!id)
    {
      mu_error ("%s", mu_strerror (errno));
      return 1;
    }
  id->dev = dev;
  id->inode = ino;
  return mu_list_append (idlist, id);
}

static struct safety_checker *
_find_safety_checker (const char *name)
{
  struct safety_checker *pck;
  for (pck = file_safety_check_tab; pck->flag; pck++)
    if (strcmp (pck->name, name) == 0)
      return pck;
  return NULL;
}

const char *
mu_file_safety_code_to_name (int code)
{
  struct safety_checker *pck;
  for (pck = file_safety_check_tab; pck->flag; pck++)
    {
      if (pck->flag == code)
	return pck->name;
    }
  return NULL;
}

int
mu_file_safety_name_to_code (const char *name, int *pcode)
{
  struct safety_checker *pck = _find_safety_checker (name);
  if (pck)
    {
      *pcode = pck->flag;
      return 0;
    }
  return MU_ERR_NOENT;
}

int
mu_file_safety_name_to_error (const char *name, int *pcode)
{
  struct safety_checker *pck = _find_safety_checker (name);
  if (pck)
    {
      *pcode = pck->err;
      return 0;
    }
  return MU_ERR_NOENT;
}

int
mu_file_safety_check (const char *filename, int mode,
		      uid_t uid,
		      mu_list_t idlist)
{
  struct file_check_buffer buf;

  if (!filename)
    return EFAULT;
  memset (&buf, 0, sizeof (buf));
  if (lstat (filename, &buf.filst) == 0)
    {
      struct safety_checker *pck;

      if (idlist)
	{
	  mu_list_set_destroy_item (idlist, mu_list_free_item);
	  mu_list_set_comparator (idlist, file_id_cmp);
	  if (file_id_lookup (idlist, buf.filst.st_dev, buf.filst.st_ino) == 0)
	    return MU_ERR_EXISTS;
	}

      if ((mode & MU_FILE_SAFETY_OWNER_MISMATCH) && uid != buf.filst.st_uid)
	return MU_ERR_PERM_OWNER_MISMATCH;

      for (pck = file_safety_check_tab; pck->flag; pck++)
	if (mode & pck->flag)
	  {
	    if (pck->cdir && !buf.cdir)
	      {
		char *dirname, *p;

		p = strrchr (filename, '/');
		if (!p)
		  dirname = strdup (".");
		else if (p == filename)
		  dirname = strdup ("/");
		else
		  {
		    size_t len = p - filename;
		    dirname = malloc (len + 1);
		    if (dirname)
		      {
			memcpy (dirname, filename, len);
			dirname[len] = 0;
		      }
		  }
		if (!dirname)
		  return ENOMEM;
		if (stat (dirname, &buf.dirst))
		  return errno;
		buf.cdir = 1;
	      }
	    if ((pck->fun && pck->fun (&buf)) ||
		(buf.filst.st_mode & pck->mode))
	      return pck->err;
	  }
      if (idlist)
	file_id_remember (idlist, buf.filst.st_dev, buf.filst.st_ino);
      return 0;
    }
  return errno;
}

int
mu_file_mode_to_safety_criteria (int mode)
{
  int fl = 0;
  struct safety_checker *pck;

  for (pck = file_safety_check_tab; pck->name; pck++)
    if (!(mode & pck->mode))
      fl |= pck->flag;
  return fl;
}

int
mu_safety_criteria_to_file_mode (int crit)
{
  int mode = 0666;
  struct safety_checker *pck;

  for (pck = file_safety_check_tab; pck->name; pck++)
    if (pck->flag && (crit & pck->flag))
      mode &= ~pck->mode;
  return mode;
}
