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

#include "mailbox.h"
#include "unixmbox.h"

#ifdef _HAVE_ERRNO_H
#include <errno.h>
#endif

#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

int _mbox_dummy1 (mailbox * mbox);
int _mbox_dummy2 (mailbox * mbox, unsigned int num);
int _mbox_dummy3 (mailbox * mbox, char *c);
char *_mbox_dummy4 (mailbox * mbox, unsigned int num);

mailbox *
mbox_open (const char *name)
{
  mailbox *mbox;
  struct stat st;

  if ( name == NULL )
    {
      errno = EINVAL;
      return NULL;
    }

  mbox = malloc (sizeof (mailbox));
  if (mbox == NULL)
    {
      errno = ENOMEM;
      return NULL;
    }

  /* Set up with default data and dummy functions */
  mbox->name = strdup (name);
  if (mbox->name == NULL)
    {
      errno = ENOMEM;
      free (mbox);
      return NULL;
    }
  /* check if we can look at file, prep for checks later in function */
  if (stat (mbox->name, &st) == -1)
    {
      free (mbox->name);
	  free (mbox);
	  return NULL; /* errno set by stat() */
    }

  mbox->messages = 0;
  mbox->num_deleted = 0;
  mbox->sizes = NULL;
  mbox->_data = NULL;
  mbox->_close = _mbox_dummy1;
  mbox->_delete = _mbox_dummy2;
  mbox->_undelete = _mbox_dummy2;
  mbox->_expunge = _mbox_dummy1;
  mbox->_add_message = _mbox_dummy3;
  mbox->_is_deleted = _mbox_dummy2;
  mbox->_lock = _mbox_dummy2;
  mbox->_get_body = _mbox_dummy4;
  mbox->_get_header = _mbox_dummy4;

  if (S_ISREG (st.st_mode))
    {
      if (unixmbox_open (mbox) == 0)
        return mbox;
      else
        {
          /*
           * Check errno to find out why it failed, if it's simply not a
           * unix mbox format message, then try other mailbox types,
           * otherwise, leave errno set and return NULL
           */
          if (errno == EBADMSG)
            errno = ENOSYS; /* no other mailboxes supported right now */
        }
    }
  else if (S_ISDIR (st.st_mode))
    {
      /* for example...
         if (maildir_open (mbox, name) == 1)
           return mbox;
       */
      errno = ENOSYS;
    }
  else
    errno = EINVAL; /* neither directory nor file, so bomb */

  free (mbox->name);
  free (mbox);
  return NULL;
}

/*
 * Gets the contents of a header field
 */
char *
mbox_header_line (mailbox *mbox, unsigned int num, const char *header)
{
  char *full, *tmp, *line;
  int try = 1;
  size_t i = 0, j = 0;
  size_t len, lh;
  
  if ( mbox == NULL || header == NULL )
    {
      errno = EINVAL;
      return NULL;
    }

  full = mbox_get_header (mbox, num);
  if (full == NULL)
    return NULL;
 
  lh = strlen (header);
  len = strlen (full);
  tmp = NULL;

  /* First get the appropriate line at the beginning */
  for (i=0; i < len-(lh+2); i++)
    {
      if (try == 1)
	{
	  if (!strncasecmp(&full[i], header, lh) && full[i+lh] == ':')
	    {
	      tmp = strdup (&full[i+lh+2]);
		  if (tmp == NULL)
            {
              free(full);
              return NULL;
            }
          break;
	    }
	  else
	    try = 0;
	}
      else if (full[i] == '\n')
	try = 1;
      else
	try = 0;
    }

  /* FIXME: hmm, no valid header found, what should errno be? */
  if (tmp == NULL)
    {
      free(full);
      return NULL;
    }
  /* Now trim the fat */
  len = strlen (tmp);
  for (i = 0; i < len; i++)
    {
      if (tmp[i] == '\n' && i < (len - 1) && isspace (tmp[i+1]))
	{
	  if (tmp[i+1] == '\t')
	    tmp[i + 1] = ' ';
	  for (j = i; j < len; j++)
	    tmp[j] = tmp[j+1];
	}
      else if (tmp[i] == '\n')
	{
	  tmp[i] = '\0';
      break;
	}
    }
  line = strdup (tmp);
  free (tmp);
  free (full);
  return line;
}

/*
 * Gets first LINES lines from message body
 */
char *
mbox_body_lines (mailbox *mbox, unsigned int num, unsigned int lines)
{
  char *full, *buf = NULL;
  int i=0, line = 0, len;
  if (mbox == NULL)
    {
      errno = EINVAL;
	  return NULL;
    }
  if (lines == 0)
    return strdup ("");
  full = mbox_get_body (mbox, num);
  if (full == NULL)
    return NULL;
  len = strlen (full);
  for (i=0; i < len && line < lines; i++)
    {
      if (full[i] == '\n')
	{
	  line++;
	  if (line >= lines)
	    {
	      full[i+1] = '\0';
	      buf = strdup (full);
          break;
	    }
	}
    }
  if (buf == NULL)
    buf = strdup (full);
  free (full);
  return buf;
}

/*
 * Bogus functions for unimplemented functions that return int
 */
int
_mbox_dummy1 (mailbox * mbox)
{
  errno = ENOSYS;
  return -1;
}

int 
_mbox_dummy2 (mailbox * mbox, unsigned int num)
{
  return _mbox_dummy1 (mbox);
}
int 
_mbox_dummy3 (mailbox * mbox, char *c)
{
  return _mbox_dummy1 (mbox);
}

/*
 * Bogus function for unimplemented functions that return char *
 */
char *
_mbox_dummy4 (mailbox * mbox, unsigned int num)
{
  errno = ENOSYS;
  return NULL;
}

