/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include <config.h> 
#endif

#include "maildir.h"

/*
 * Opens a mailbox
 */
int
maildir_open (mailbox * mbox)
{
  int i;
  int alloced = 0;
  maildir_data *data;
  struct dirent *entry;
  char old_dir[PATH_MAX];
  struct utsname *utsinfo;
  uname (utsinfo);
  
  getcwd (old_dir, PATH_MAX);
  chdir (mbox->name);
  
  data->new = opendir ("new");
  data->tmp = opendir ("tmp");
  data->cur = opendir ("cur");
  data->time = time (NULL);
  data->pid = getpid ();
  data->sequence = 0;
  /* data->host = strdup (utsinfo->sysname); */
  data->messages = malloc (10 * sizeof (maildir_message));
  alloced = 10;
  
  /* process the new directory */
  while (entry = readdir (data->new))
    {
      /* no dot files */
      if (entry->d_name[0] != '.')
	{
	  if (mbox->messages >= alloced)
	    {
	      alloced *= 2;
	      data->messages = realloc (data->messages,
					alloced * sizeof (maildir_message));
	      /* check */
	    }
	  data->messages[mbox->messages].deleted = 0;
	  data->messages[mbox->messages].info = NULL;
	  data->messages[mbox->messages].location = new;
	  data->messages[mbox->messages].file = strdup (entry->d_name);
	  /* check */
	  mbox->messages++;
	}
    }
  
  /* then the cur directory */
  while (entry = readdir (data->cur))
    {
      if (entry->d_name[0] != '.')
	{
	  if (mbox->messages >= alloced)
	    {
	      alloced *= 2;
	      data->messages = realloc (data->messages,
				alloced * sizeof (maildir_message));
	      /* check */
	    }
	  data->messages[mbox->messages].deleted = 0;
	  data->messages[mbox->messages].location = cur;
	  data->messages[mbox->messages].file = strdup (entry->d_name);
	  /* check */
	  /* data->messages[i].info = parse_for_info (entry->d_name); */
	  mbox->messages++;
	}
    }
  
  for (i=0; i < mbox->messages; i++)
    {
      FILE *f;
      char buf[80];
      chdir (mbox->name);
      if (data->messages[i].location == new)
	chdir ("new");
      else if (data->messages[i].location == cur)
	chdir ("cur");
      f = fopen (data->messages[i].file, "r");
      data->messages[i].body = 0;
      while (fgets (buf, 80, f))
	if (!strncmp (buf, "\r\n", 2) || strlen (buf) <= 1)
	  fgetpos (f, &(data->messages[i].body));
      fclose (f);
    }
  
  mbox->_data = data;
  mbox->_close = maildir_close;
  mbox->_delete = maildir_delete;
  mbox->_undelete = maildir_undelete;
  mbox->_expunge = maildir_expunge;
  mbox->_add_message = maildir_add_message;
  mbox->_is_deleted = maildir_is_deleted;
  mbox->_lock = maildir_lock;
  mbox->_get_body = maildir_get_body;
  mbox->_get_header = maildir_get_header;
  
  chdir (old_dir);
  return 0;
}

/*
 * Closes and unlocks a mailbox, does not expunge
 */
int
maildir_close (mailbox * mbox)
{
  /* FIXME: hack from unixmbox */
  return 0;
}

/*
 * Marks a message for deletion
 */
int 
maildir_delete (mailbox * mbox, unsigned int num)
{
  maildir_data *data;

  if (mbox == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (num > mbox->messages || mbox_is_deleted (mbox, num))
    {
      errno = ERANGE;
      return -1;
    }

  data = mbox->_data;
  data->messages[num].deleted = 1;
  mbox->num_deleted++;

  return 0;
}

/*
 * Unmark a message for deletion
 */
int 
maildir_undelete (mailbox * mbox, unsigned int num)
{
  maildir_data *data;

  if (mbox == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (num > mbox->messages || !mbox_is_deleted (mbox, num))
    {
      errno = ERANGE;
      return -1;
    }

  data = mbox->_data;
  data->messages[num].deleted = 0;
  mbox->num_deleted--;

  return 0;
}

/*
 * Updates a mailbox to remove message marked for deletion
 */
int 
maildir_expunge (mailbox * mbox)
{
  maildir_data *data;
  int i = 0;
  char old_dir[PATH_MAX];

  if (mbox == NULL)
    {
      errno = EINVAL;
	  return -1;
    }

  getcwd (old_dir, PATH_MAX);
  chdir (mbox->name);

  if (mbox->num_deleted)
    {
      data = mbox->_data;
      
      for (i = 0; i < mbox->messages; i++)
	{
	  if (data->messages[i].deleted == 0)
	    {
	      unlink (data->messages[i].file);
	      mbox->num_deleted--;
	    }
	}
    }
  /* reorder messages? */
  
  chdir (old_dir);
  return 0;
}

/*
 * Determines whether or a not a message is marked for deletion
 */
int
maildir_is_deleted (mailbox * mbox, unsigned int num)
{
  maildir_data *data;

  if (mbox == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  if (mbox->num_deleted == 0)
    return 0;
  data = mbox->_data;
  return (data->messages[num].deleted == 1);
}

int
maildir_is_updated (mailbox *mbox)
{
  struct stat st;
  maildir_data *data;

  if (mbox == NULL)
    {
      errno = EINVAL;
      return -1;
    }
  if (stat (mbox->name, &st) == -1)
      return -1;
  data = mbox->_data;
  return (st.st_mtime > data->last_mod_time);
}

/*
 * Adds a message to the mailbox
 */
int 
maildir_add_message (mailbox * mbox, char *message)
{
  /* 
   * FIXME: please write me
   * http://www.qmail.org/man/man5/maildir.html
   */
  errno = ENOSYS;
  return -1;
}

/*
 * Returns a message body
 */
char *
maildir_get_body (mailbox * mbox, unsigned int num)
{
  maildir_data *data;
  char *buf;
  char old_dir[PATH_MAX];
  FILE *f;
  struct stat st;
  size_t size;

  if (mbox == NULL)
    {
      errno = EINVAL;
	  return NULL;
    }
  if (num > mbox->messages || num < 0)
    {
      errno = ERANGE;
      return NULL;
    }

  data = mbox->_data;
  getcwd (old_dir, PATH_MAX);
  chdir (mbox->name);

  if (data->messages[num].location == cur)
    chdir ("cur");
  else if (data->messages[num].location == tmp)
    chdir ("tmp");
  else if (data->messages[num].location == new)
    chdir ("new");

  f = fopen (data->messages[num].file, "r");
  if (f == NULL)
    {
      chdir (old_dir);
      perror (NULL);
      return NULL;
    }

  stat (data->messages[num].file, &st);
  size = st.st_size - data->messages[num].body + 1;

  buf = malloc (sizeof (char) * size);
  if (buf == NULL)
    {
      fclose (f);
      chdir (old_dir);
      errno = ENOMEM;
      return NULL;
    }

  memset (buf, 0, size);
  if (fsetpos (f, &(data->messages[num].body)) == -1)
    {
      fclose (f);
      chdir (old_dir);
      free (buf);
      return NULL;
    }

  if (fread (buf, sizeof (char), size-1, f) < size)
    {
      fclose (f);
      chdir (old_dir);
      free (buf);
      return NULL;
    }
  
  fclose (f);
  chdir (old_dir);
  return buf;
}

/*
 * Returns just the header of a message
 */
char *
maildir_get_header (mailbox * mbox, unsigned int num)
{
  maildir_data *data;
  char *buf;

  if ( mbox == NULL )
    {
      errno = EINVAL;
	  return NULL;
    }

  if (num > mbox->messages)
    {
      errno = ERANGE;
      return NULL;
    }

  data = mbox->_data;
  buf = NULL; /* FIXME: read the file until data->message[num].body */
  return buf;
}

/*
 * Sets lock mode for a mailbox
 * FIXME: What to do for Maildir? no locking?
 */
int
maildir_lock (mailbox *mbox, mailbox_lock_t mode)
{
  return 0;
}

/* FIXME: not implemented */
int
maildir_scan (mailbox *mbox)
{
  errno = ENOSYS;
  return -1;
}
