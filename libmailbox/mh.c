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

#include "mh.h"

/*
 * Opens a mailbox
 */
int
mh_open (mailbox * mbox)
{
  int i;
  char old_dir[PATH_MAX];
  struct dirent *entry;
  mh_data *data;

  if(mbox == NULL)
    return EINVAL;

  if(getcwd (old_dir, PATH_MAX) == NULL)
    old_dir[0] = '\0';
  chdir (mbox->name);

  data = malloc (sizeof (mh_data));
  data->dir = opendir (mbox->name);
  if(data->dir == NULL)
    return errno; /* set by opendir() */

  /* process directory */
  while (entry = readdir (data->dir))
    {
      unsigned long seq_num;
      char *foo = NULL;
      if (entry->d_name[0] == '.')
	{
	  if (strcmp(entry->d_name, ".mh_sequences" == 0))
	    /* TODO: deal with mh sequence files */;
	  continue;
	}
      if (entry->d_name[0] == ',')
	/* file marked for deletion */;

      /* TODO: handle ERANGE */
      seq_num = strtoul (entry->d_name, &foo, 10);
      if (*foo != '\0') /* invalid sequence number */
	continue; /* TODO: handle this better? */
      printf ("message: %ld\r", seq_num);
      mbox->messages++;
    }
  
  mbox->_data = data;
#if 0
  mbox->_close = mh_close;
  mbox->_delete = mh_delete;
  mbox->_undelete = mh_undelete;
  mbox->_expunge = mh_expunge;
  mbox->_add_message = mh_add_message;
  mbox->_is_deleted = mh_is_deleted;
  mbox->_lock = mh_lock;
  mbox->_get_body = mh_get_body;
  mbox->_get_header = mh_get_header;
#endif
  chdir(old_dir);

  fprintf(stderr, "%d messages read\n", mbox->messages);

  return 0;
}

#if 0
/*
 * Closes and unlocks a mailbox, does not expunge
 */
int
mh_close (mailbox * mbox)
{
  /* FIXME: hack from unixmbox */
  return 0;
}

/*
 * Marks a message for deletion
 */
int 
mh_delete (mailbox * mbox, unsigned int num)
{
  mh_data *data;

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
mh_undelete (mailbox * mbox, unsigned int num)
{
  mh_data *data;

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
mh_expunge (mailbox * mbox)
{
  mh_data *data;
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
mh_is_deleted (mailbox * mbox, unsigned int num)
{
  mh_data *data;

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
mh_is_updated (mailbox *mbox)
{
  struct stat st;
  mh_data *data;

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
mh_add_message (mailbox * mbox, char *message)
{
  /* 
   * FIXME: please write me
   * http://www.qmail.org/man/man5/mh.html
   */
  errno = ENOSYS;
  return -1;
}

/*
 * Returns a message body
 */
char *
mh_get_body (mailbox * mbox, unsigned int num)
{
  mh_data *data;
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
mh_get_header (mailbox * mbox, unsigned int num)
{
  mh_data *data;
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
 * FIXME: What to do for Mh? no locking?
 */
int
mh_lock (mailbox *mbox, mailbox_lock_t mode)
{
  return 0;
}

/* FIXME: not implemented */
int
mh_scan (mailbox *mbox)
{
  errno = ENOSYS;
  return -1;
}
#endif
