/* copyright and license info go here */

#include "unixmbox.h"

/*
 * Opens and locks a mailbox
 */
int
unixmbox_open (mailbox * mbox)
{
  char buf[80];
  unsigned int max_count = 10;
  int mess = 0;
  unixmbox_data *data = malloc (sizeof (unixmbox_data));

  if (data == NULL)
    {
      errno = ENOMEM;
      free (data);
      return -1;
    }

  mbox->_data = data;
  data->file = fopen (mbox->name, "r+");
  /* FIXME: do a good, NFS safe, file lock here, with error handling */
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

  while (fgets (buf, 80, data->file))
    {
      if (!strncmp (buf, "From ", 5))
	{
	  /* Beginning of a header */
	  while (strchr (buf, '\n') == NULL)
	    fgets (buf, 80, data->file);

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

  mbox->_close = unixmbox_close;
  mbox->_delete = unixmbox_delete;
  mbox->_undelete = unixmbox_undelete;
  mbox->_expunge = unixmbox_expunge;
  mbox->_add_message = unixmbox_add_message;
  mbox->_is_deleted = unixmbox_is_deleted;
  mbox->_get_body = unixmbox_get_body;
  mbox->_get_header = unixmbox_get_header;

  return 0;
}

/*
 * Closes and unlocks a mailbox, does not expunge
 */
int
unixmbox_close (mailbox * mbox)
{
  unixmbox_data *data = mbox->_data;
  /* FIXME: good file unlocking, to go along with the locking above */
  fclose (data->file);
  free (data->messages);
  free (mbox->sizes);
  free (data);
  free (mbox);
  return 0;
}

/*
 * Marks a message for deletion
 */
int 
unixmbox_delete (mailbox * mbox, int num)
{
  unixmbox_data *data = mbox->_data;
  if (num > mbox->messages || mbox_is_deleted (mbox, num))
    {
      errno = ERANGE;
      return -1;
    }
  else
    {
      data->messages[num].deleted = 1;
      mbox->num_deleted++;
    }
  return 0;
}

/*
 * Unmark a message for deletion
 */
int 
unixmbox_undelete (mailbox * mbox, int num)
{
  unixmbox_data *data = mbox->_data;
  if (num > mbox->messages || !mbox_is_deleted (mbox, num))
    {
      errno = ERANGE;
      return -1;
    }
  else
    {
      data->messages[num].deleted = 0;
      mbox->num_deleted--;
    }
  return 0;
}

/*
 * Updates a mailbox to remove message marked for deletion
 * FIXME: BROKEN! DOES NOT WORK! BLOWS UP REAL GOOD!
 */
int 
unixmbox_expunge (mailbox * mbox)
{
  unixmbox_data *data = mbox->_data;
  int i = 0, size = 0;
  char *buf;
  fpos_t lastpos;

  data->file = freopen (mbox->name, "r+", data->file);
  if (data->file == NULL)
    {
      /* error handling */
    }
  fgetpos (data->file, &lastpos);

  for (i = 0; i < mbox->messages; i++)
    {
      if (data->messages[i].deleted == 0)
	{
	  fgetpos (data->file, &lastpos);
	  fsetpos (data->file, &(data->messages[i].header));
	  read (fileno (data->file), buf,
		data->messages[i + 1].header - data->messages[i].header);
	  fprintf (data->file, "%s", buf);
	  size += strlen (buf);
	  free (buf);
	}
    }
  ftruncate (fileno (data->file), size);
  return 0;
}

/*
 * Determines whether or a not a message is marked for deletion
 */
int
unixmbox_is_deleted (mailbox * mbox, int num)
{
  unixmbox_data *data = mbox->_data;
  return (data->messages[num].deleted == 1);
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
unixmbox_get_body (mailbox * mbox, int num)
{
  unixmbox_data *data = mbox->_data;
  int size = data->messages[num].end - data->messages[num].body;
  char *buf = malloc ((1 + size) * sizeof (char));
  if (buf == NULL)
    {
      errno = ENOMEM;
      free (buf);
      return NULL;
    }
  else if (num > mbox->messages || num < 0)
    {
      errno = ERANGE;
      free (buf);
      return NULL;
    }

  memset (buf, 0, size + 1);
  fsetpos (data->file, &(data->messages[num].body));
  fread (buf, size, sizeof (char), data->file);
  return buf;
}

/*
 * Returns just the header of a message
 */
char *
unixmbox_get_header (mailbox * mbox, int num)
{
  unixmbox_data *data = mbox->_data;
  int size = (data->messages[num].body - 1) - data->messages[num].header ;
  char *buf = malloc ((1 + size) * sizeof (char));
  if (buf == NULL)
    {
      errno = ENOMEM;
      free (buf);
      return NULL;
    }
  else if (num > mbox->messages || num < 0)
    {
      errno = ERANGE;
      free (buf);
      return NULL;
    }

  memset (buf, 0, size + 1);
  fsetpos (data->file, &(data->messages[num].header));
  fread (buf, size, sizeof (char), data->file);
  return buf;
}
