/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#include "guimb.h"

void
util_error (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "guimb: ");
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
}

int
util_tempfile (char **namep)
{
  char *filename;
  char *tmpdir;
  int fd;

  /* We have to be extra careful about opening temporary files, since we
     may be running with extra privilege i.e setgid().  */
  tmpdir = (getenv ("TMPDIR")) ? getenv ("TMPDIR") : "/tmp";

  filename = malloc (strlen (tmpdir) + /*'/' */ 1 + /* "muXXXXXX" */ 8 + 1);
  if (!filename)
    return -1;
  sprintf (filename, "%s/muXXXXXX", tmpdir);

#ifdef HAVE_MKSTEMP
  {
    int save_mask = umask (077);
    fd = mkstemp (filename);
    umask (save_mask);
  }
#else
  if (mktemp (filename))
    fd = open (filename, O_CREAT | O_EXCL | O_RDWR, 0600);
  else
    fd = -1;
#endif

  if (fd == -1)
    {
      util_error ("Can not open temporary file: %s", strerror (errno));
      free (filename);
      return -1;
    }

  if (namep)
    *namep = filename;
  else
    {
      unlink (filename);
      free (filename);
    }

  return fd;
}

char *
util_get_sender (int msgno)
{
  header_t header = NULL;
  address_t addr = NULL;
  message_t msg = NULL;
  char buffer[512];

  mailbox_get_message (mbox, msgno, &msg);
  message_get_header (msg, &header);
  if (header_get_value (header, MU_HEADER_FROM, buffer, sizeof (buffer), NULL)
      || address_create (&addr, buffer))
    {
      envelope_t env = NULL;
      message_get_envelope (msg, &env);
      if (envelope_sender (env, buffer, sizeof (buffer), NULL)
	  || address_create (&addr, buffer))
	{
	  util_error ("can't determine sender name (msg %d)", msgno);
	  return NULL;
	}
    }

  if (address_get_email (addr, 1, buffer, sizeof (buffer), NULL))
    {
      util_error ("can't determine sender name (msg %d)", msgno);
      address_destroy (&addr);
      return NULL;
    }

  address_destroy (&addr);
  return strdup (buffer);
}
