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

#include <mailbox0.h>
#include <registrar0.h>

#include <string.h>
#include <errno.h>
#include <sys/stat.h>

static int mailbox_mbox_init (mailbox_t *mbox, const char *name);
static void mailbox_mbox_destroy (mailbox_t *mbox);

struct mailbox_registrar _mailbox_mbox_registrar =
{
  "UNIX_MBOX/Maildir/MMDF",
  mailbox_mbox_init, mailbox_mbox_destroy
};

/*
  Caveat there is no specific URL for file mailbox or simple path name,
    <path_name>
    file:<path_name>

  It would be preferrable to use :
    maildir:<path>
    unix:<path>
    mmdf:<path>
  This would eliminate heuristic discovery that would turn
  out to be wrong.
*/

static int
mailbox_mbox_init (mailbox_t *mbox, const char *name)
{
  struct stat st;
  size_t len;

  if (name == NULL || mbox == NULL)
    return EINVAL;

  len = strlen (name);
  if (len >= 5 &&
      (name[0] == 'f' || name[0] == 'F') &&
      (name[1] == 'i' || name[1] == 'I') &&
      (name[2] == 'l' || name[2] == 'L') &&
      (name[3] == 'e' || name[3] == 'E') &&
      name[4] == ':')
    {
      name += 5;
    }
  /*
   * If they want to creat ?? should they  know the type ???
   * What is the best course of action ??
   * For the default is unix if the file does not exist.
   */
  if (stat (name, &st) < 0)
    return _mailbox_unix_registrar._init (mbox, name);

  if (S_ISREG (st.st_mode))
    {
      /*
	FIXME: We should do an open() and try
	to do a better reconnaissance of the type,
	maybe MMDF.  For now assume Unix MBox */
#if 0
      char head[6];
      ssize_t cout;
      int fd;

      fd = open (name, O_RDONLY);
      if (fd == -1)
	{
	  /* Oops !! wrong permission ? file deleted ? */
	  return errno; /* errno set by open () */
	}

      /* Read a small chunck */
      count = read (fd, head, sizeof(head));

      /* Try Unix Mbox */

      /* FIXME:
	 What happen if the file is empty ???
	 Do we default to Unix ??
      */
      if (count == 0) /*empty file*/
	{
	  close (fd);
	  return _mailbox_unix_registrar._init (mbox, name);
	}

      if (count >= 5)
	{
	  if (strncmp (head, "From ", 5) == 0)
	    {
	      /* This is a Unix Mbox */
	      close (fd);
	      return _mailbox_unix_registrar._init (mbox, name);
	    }
	}

      /* Try MMDF */
      close (fd);
#endif
      return _mailbox_unix_registrar._init (mbox, name);
    }
  /* Is that true ?  Are all directories Maildir ?? */
  else if (S_ISDIR (st.st_mode))
    return _mailbox_maildir_registrar._init (mbox, name);

  /* Why can't a mailbox be FIFO ? or a DOOR/Portal ??? */
  return EINVAL;
}

static void
mailbox_mbox_destroy (mailbox_t *pmbox)
{
  if (pmbox && *pmbox)
    _mailbox_unix_registrar._destroy (pmbox);
}
