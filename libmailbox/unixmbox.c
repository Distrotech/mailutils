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

#include "unixmbox.h"

/*
 * Opens a mailbox
 */
int
unixmbox_open (mailbox * mbox)
{
  char buf[80];
  unsigned int max_count = 10;
  int mess = 0;
  unixmbox_data *data;
  struct stat st;
  
  if (mbox == NULL)
    {
      errno = EINVAL;
	  return -1;
    }

  data = malloc (sizeof (unixmbox_data));
  if (data == NULL)
    {
      errno = ENOMEM;
      free (data);
      return -1;
    }

  mbox->_data = data;
  data->file = fopen (mbox->name, "r+");
  if (data->file == NULL)
    {
      /* errno is set by fopen() */
      free (data);
      return -1;
    }

  data->messages = malloc (sizeof (unixmbox_message) * max_count);
  mbox->sizes = malloc (sizeof (int) * max_count);
  if (data->messages == NULL || mbox->sizes == NULL)
    {
      unixmbox_close (mbox);
      errno = ENOMEM;
      return -1;
    }
  if (stat (mbox->name, &st) == -1)
    {
      unixmbox_close (mbox);
	  return -1;
    }
  data->last_mod_time = st.st_mtime;

  if (fgets (buf, 80, data->file) == NULL)
    {
      if (feof(data->file))
        goto END; /* empty file, no messages */
	  unixmbox_close (mbox);
	  return -1;
    }
  if (strncmp (buf, "From ", 5))
    {
      /* This is NOT an mbox file */
      unixmbox_close (mbox);
      errno = EBADMSG; /* use this to signify wrong mbox type */
      return -1;
    }

  do
    {
      if (!strncmp (buf, "From ", 5))
	{
	  /* Beginning of a header */
	  while (strchr (buf, '\n') == NULL)
	    if (fgets (buf, 80, data->file) == NULL) /* eat the From line */
          {
            if (feof (data->file))
              errno = EIO; /* corrupted mailbox? */
            unixmbox_close (mbox);
            return -1;
          }

	  mbox->messages++;

	  if (mbox->messages > max_count)
	    {
	      max_count = mbox->messages * 2;
	      data->messages = realloc (data->messages,
				     max_count * sizeof (unixmbox_message));
	      mbox->sizes = realloc (mbox->sizes, max_count * sizeof (int));
	      if (data->messages == NULL || mbox->sizes == NULL)
		{
		  unixmbox_close (mbox);
		  errno = ENOMEM;
		  return -1;
		}
	    }

	  fgetpos (data->file, &(data->messages[mbox->messages - 1].header));
	  mbox->sizes[mbox->messages - 1] = 0;
	  data->messages[mbox->messages - 1].deleted = 0;
	  mess = 0;
	}
      else if (((!strncmp (buf, "\r\n", 2) || strlen (buf) <= 1)) && mess == 0)
	{
	  /* Beginning of a body */
	  fgetpos (data->file, &(data->messages[mbox->messages - 1].body));
	  mbox->sizes[mbox->messages - 1] += strlen (buf) + 1;
	  mess = 1;
	}
      else
	{
	  mbox->sizes[mbox->messages - 1] += strlen (buf) + 1;
	  fgetpos (data->file, &(data->messages[mbox->messages - 1].end));
	}
    }
  while (fgets (buf, 80, data->file));

  if (!feof (data->file)) /* stopped due to error, not EOF */
    {
      unixmbox_close (mbox);
      return -1; /* errno is set */
    }

END:
  mbox->_close = unixmbox_close;
  mbox->_delete = unixmbox_delete;
  mbox->_undelete = unixmbox_undelete;
  mbox->_expunge = unixmbox_expunge;
  mbox->_add_message = unixmbox_add_message;
  mbox->_is_deleted = unixmbox_is_deleted;
  mbox->_lock = unixmbox_lock;
  mbox->_get_body = unixmbox_get_body;
  mbox->_get_header = unixmbox_get_header;
#ifdef TESTING
  mbox->_tester = unixmbox_tester;
#endif

  return 0;
}

/*
 * Closes and unlocks a mailbox, does not expunge
 */
int
unixmbox_close (mailbox * mbox)
{
  unixmbox_data *data;
  
  if (mbox == NULL)
    {
      errno = EINVAL;
	  return -1;
    }
  data = mbox->_data;
  unixmbox_lock (mbox, MO_ULOCK);
  if (data->file)
    fclose (data->file);
  free (data->messages);
  free (mbox->sizes);
  free (data);
  return 0;
}

/*
 * Marks a message for deletion
 */
int 
unixmbox_delete (mailbox * mbox, unsigned int num)
{
  unixmbox_data *data;

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
unixmbox_undelete (mailbox * mbox, unsigned int num)
{
  unixmbox_data *data;

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
 * FIXME: BROKEN! DOES NOT WORK! BLOWS UP REAL GOOD!
 */
int 
unixmbox_expunge (mailbox * mbox)
{
  unixmbox_data *data;
  int i = 0;
  size_t size = 0, size_read = 0;
  char *buf = NULL;
  int file;
  int deletion_needed = 0; /* true when a deleted message has been found */
  size_t buff_size = 0;
  size_t tmp = 0;

  if (mbox == NULL)
    {
      errno = EINVAL;
	  return -1;
    }
  if (mbox->num_deleted)
  {
    data = mbox->_data;
    fclose(data->file);
    /* error handling */
    data->file = NULL;
    file = open(mbox->name, O_RDWR);
    /* error handling */

  for (i = 0; i < mbox->messages; i++)
    {
      if (data->messages[i].deleted == 0)
	{
      if (deletion_needed)
      {
        tmp = mbox->sizes[i];
		if (tmp > buff_size)
          {
            buff_size = tmp;
            buf = realloc (buf, tmp);
			/* error checking */
          }
        lseek (file, data->messages[i].header, SEEK_SET);
        size_read = read (file, buf, tmp);
		/* error checking */
		lseek (file, size, SEEK_SET);
        write (file, buf, size_read);
		/* error checking */
      	size += size_read;
      }
      else
      {
        size += mbox->sizes[i];
      }
	}
	  else
    {
        deletion_needed = 1;
    }
    }
    close (file);
    truncate (mbox->name, size);
    free (buf);
  }
  return 0;
}

/*
 * Determines whether or a not a message is marked for deletion
 */
int
unixmbox_is_deleted (mailbox * mbox, unsigned int num)
{
  unixmbox_data *data;

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
unixmbox_is_updated (mailbox *mbox)
{
  struct stat st;
  unixmbox_data *data;

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
unixmbox_add_message (mailbox * mbox, char *message)
{
  /* 
   * FIXME: please write me
   * move to end of file and write text, don't forget to update
   * mbox->messages and mbox->_data->messages
   */
  errno = ENOSYS;
  return -1;
}

/*
 * Returns a message body
 */
char *
unixmbox_get_body (mailbox * mbox, unsigned int num)
{
  unixmbox_data *data;
  size_t size;
  char *buf;

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
  size = data->messages[num].end - data->messages[num].body;
  buf = malloc ((1 + size) * sizeof (char));
  if (buf == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  memset (buf, 0, size + 1);
  if (fsetpos (data->file, &(data->messages[num].body)) == -1)
    {
      free (buf);
      return NULL;
    }
  if (fread (buf, sizeof (char), size, data->file) < size)
    {
      free (buf);
      return NULL;
    }
  return buf;
}

/*
 * Returns just the header of a message
 */
char *
unixmbox_get_header (mailbox * mbox, unsigned int num)
{
  unixmbox_data *data;
  size_t size;
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
  size = (data->messages[num].body - 1) - data->messages[num].header;
  buf = malloc ((1 + size) * sizeof (char));
  if (buf == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  memset (buf, 0, size + 1);

  if (fsetpos (data->file, &(data->messages[num].header)) == -1)
    {
      free (buf);
      return NULL;
    }
  if (fread (buf, sizeof (char), size, data->file) < size)
    {
      free (buf);
      return NULL;
    }
  return buf;
}

/*
 * Sets lock mode for a mailbox
 * FIXME: this doesn't do anything, really
 * Get locking code from Procmail and/or Exim
 */
int
unixmbox_lock (mailbox *mbox, mailbox_lock_t mode)
{
  unixmbox_data *data = mbox->_data;
  data->lockmode = mode;
  return 0;
}

/* FIXME: not implemented */
int
unixmbox_scan (mailbox *mbox)
{
  errno = ENOSYS;
  return -1;
}

#ifdef TESTING
void unixmbox_tester (mailbox *mbox, unsigned int num)
{
  unixmbox_data *data = mbox->_data;
  if (!data || num > mbox->messages || num < 0)
    return;
  printf ("Message size: %u\n", mbox->sizes[num]);
  printf ("Message length: %lu\n",
        data->messages[num].end - data->messages[num].header);
}
#endif
