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
#include <config.h>
#endif


#include <url_mbox.h>
#include <mbx_mbox.h>
#include <mbx_unix.h>
#include <mbx_mdir.h>
#include <string.h>

#include <errno.h>
#include <sys/stat.h>

struct mailbox_type _mailbox_mbox_type =
{
  "UNIX_MBOX/Maildir/MMDF",
  (int)&_url_mbox_type, &_url_mbox_type,
  mailbox_mbox_init, mailbox_mbox_destroy
};

/*
  if there is no specific URL for file mailbox,
    file://<path_name>

  It would be preferrable to use :
    maildir://<path>
    unix://<path>
    mmdf://<path>
  This would eliminate heuristic discovery that would turn
  out to be wrong. Caveat, there is no std URL for those
  mailbox.
*/

int
mailbox_mbox_init (mailbox_t *mbox, const char *name)
{
  struct stat st;
  char *scheme = strstr (name, "://");

  if (scheme)
    {
      scheme += 3;
      name = scheme;
    }
  /*
    If they want to creat ?? should they  know the type ???
    What is the best course of action ??
  */
  if (stat (name, &st) < 0)
    {
      return errno; /* errno set by stat () */
    }

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
	  return mailbox_unix_init (mbox, name);
	}

      if (count >= 5)
	{
	  if (strncmp (head, "From ", 5) == 0)
	    {
	      /* This is Unix Mbox */
	      close (fd);
	      return mailbox_unix_init (mbox, name);
	    }
	}

      /* Try MMDF */
      close (fd);
#endif
      return mailbox_unix_init (mbox, name);
    }
  else if (S_ISDIR (st.st_mode))
    {
      /* Is that true ?  Are all directories Maildir ?? */
      return mailbox_maildir_init (mbox, name);
    }

  /* Why can't a mailbox be FIFO ? or a DOOR/Portal ? */
  return EINVAL;
}

void
mailbox_mbox_destroy (mailbox_t *mbox)
{
  (*mbox)->mtype->_destroy (mbox);
}
