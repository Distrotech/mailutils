/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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


/* FIXME: Those headers should be in "mail.h".  */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "mail.h"

/*
 * m[ail] address...
 */

int
mail_send (int argc, char **argv)
{
  char *to = NULL, *cc = NULL, *bcc = NULL, *subj = NULL;
  char *buf = NULL;
  size_t n = 0;
  int done = 0;
  int fd;
  char *filename;
  FILE *file;
  const char *tmpdir;

  /* We have to be extra carefull about opening temporary files, since we
     may be running with extra privilege i.e setgid().  */
  tmpdir = (getenv ("TMPDIR")) ? getenv ("TMPDIR") : "/tmp";
  filename = alloca (strlen (tmpdir) + /*'/'*/1 + /* "muXXXXXX" */8 + 1);
  sprintf (filename, "%s/muXXXXXX", tmpdir);
#ifdef HAVE_MKSTEMP
  fd = mkstemp (filename);
#else
  if (mktemp (filename))
    fd = open(filename, O_CREAT|O_EXCL|O_WRONLY, 0600);
  else
    fd = -1;
#endif

  if (fd == -1)
    {
      fprintf (stderr, "Can not open temporary file");
      return 1;
    }

  file = fdopen (fd, "w");

  if (argc < 2)
    to = readline ("To: ");
  else
    {
      /* figure it out from argv */
    }

  if ((util_find_env ("askcc"))->set)
    cc = readline ("Cc: ");
  if ((util_find_env ("askbcc"))->set)
    bcc = readline ("Bcc: ");

  if ((util_find_env ("asksub"))->set)
    subj = readline ("Subject: ");
  else
    subj = (util_find_env ("subject"))->value;

  while (getline (&buf, &n, stdin) >= 0 && !done)
    {
      if (buf[0] == (util_find_env("escape"))->value[0])
	{
	  FILE *ostdout = ofile;
	  ofile = file;
	  buf[strlen(buf)-1] = '\0';
	  switch (buf[1])
	    {
	    case '!':
	      util_do_command ("!%s", &buf[2]);
	      break;
	    case '.':
	      done = 1;
	      break;
	    case ':':
	    case '-':
	      util_do_command ("%s", &buf[2]);
	      break;
	    case '?':
	      /* escape help */
	      break;
	    case 'A':
	      fprintf (ofile, "%s", (util_find_env("Sign"))->value);
	      break;
	    case 'a':
	      fprintf (ofile, "%s", (util_find_env("sign"))->value);
	      break;
	    case 'b':
	      bcc = realloc (bcc, (strlen(bcc) + strlen(buf) - 1) *
			     sizeof(char));
	      strcat (bcc, ", ");
	      strcat (bcc, &buf[3]);
	      break;
	    case 'c':
	      cc = realloc (cc, (strlen(cc) + strlen(buf) - 1) *
			    sizeof (char));
	      strcat (cc, ", ");
	      strcat (cc, &buf[3]);
	      break;
	    case 'd':
	      {
		FILE *dead = fopen (getenv("DEAD"), "r");
		char c;
		while ((c = fgetc(dead)))
		  fputc (c, file);
		fclose (dead);
	      }
	      break;
	    case 'e':
	      fclose (file);
	      ofile = ostdout;
	      util_do_command ("!%s %s", getenv("EDITOR"), filename);
	      file = fopen (filename, "a");
	      ofile = file;
	      break;
	    case 'f':
	      util_do_command ("print %s", &buf[3]);
	      break;
	    case 'F':
	      util_do_command ("Print %s", &buf[3]);
	      break;
	    case 'h':
	      /* reget Bcc, Cc, To, and Subject */
	      break;
	    case 'i':
	      fprintf (file, "%s", (util_find_env(&buf[3]))->value);
	      break;
	    case 'm':
	      /* quote messages */
	      break;
	    case 'M':
	      /* same as m with no headers ignored */
	      break;
	    case 'p':
	      fclose (file);
	      ofile = ostdout;
	      if (/* numlines (filename) > */ util_getlines())
		util_do_command ("!%s %s", getenv("PAGER"), filename);
	      else
		/* dump filename */;
	      file = fopen (filename, "a");
	      ofile = file;
	      break;
	    case 'q':
	      fclose (file);
	      rename (filename, getenv("DEAD"));
	      util_do_command ("quit");
	      break;
	    case 'r':
	    case '<':
	      /* read in a file */
	      break;
	    case 's':
	      free (subj);
	      subj = strdup (&buf[3]);
	      break;
	    case 't':
	      to = realloc (to, (strlen(to) + strlen(buf) - 1) *
			    sizeof (char));
	      strcat (to, ", ");
	      strcat (to, &buf[3]);
	      break;
	    case 'v':
	      fclose (file);
	      ofile = ostdout;
	      util_do_command ("!%s %s", getenv("VISUAL"), filename);
	      file = fopen (filename, "a");
	      ofile = file;
	      break;
	    case 'w':
	      {
		FILE *f2 = fopen (&buf[3], "a");
		/* read this file and output to f2 */
		fclose (f2);
	      }
	      break;
	    case 'x':
	      util_do_command ("quit");
	      break;
	    case '|':
	      /* pipe to &buf[3] */
	      break;
	    default:
	      fprintf (stderr, "Unknown escape %c\n", buf[0]);
	      break;
	    }
	  ofile = ostdout;
	}
      else
	fprintf (file, "%s", buf);
      fflush (file);
      /* free (buf); */
    }

  file = fopen (filename, "r");
  if (file != NULL)
    {
      mailer_t mailer;
      message_t msg = NULL;
      int status;
      char *mailer_name = alloca (strlen ("sendmail:")
				  + strlen (_PATH_SENDMAIL) + 1);
      sprintf (mailer_name, "sendmail:%s", _PATH_SENDMAIL);
      if ((status = mailer_create (&mailer, mailer_name)) != 0
          || (status = mailer_open (mailer, MU_STREAM_RDWR)) != 0)
        {
          fprintf (stderr, "%s: %s\n", mailer_name, strerror (status));
	  remove (filename);
          return 1;
        }
      message_create (&msg, NULL);

      /* Fill the header.  */
      {
	header_t header = NULL;
	message_get_header (msg, &header);
	if (to && *to != '\0')
	  header_set_value (header, MU_HEADER_TO, to, 0);
	if (cc && *cc != '\0')
	  header_set_value (header, MU_HEADER_CC, cc, 0);
	if (bcc && *bcc != '\0')
	  header_set_value (header, MU_HEADER_BCC , bcc, 0);
	if (subj && *subj != '\0')
	  header_set_value (header, MU_HEADER_SUBJECT, subj, 1);
      }

      /* Fill the body.  */
      {
	body_t body = NULL;
	stream_t stream = NULL;
	off_t offset = 0;
	message_get_body (msg, &body);
	body_get_stream (body, &stream);
	while (getline (&buf, &n, file) >= 0)
	  {
	    size_t len = strlen (buf);
	    stream_write (stream, buf, len, offset, &n);
	    offset += len;
	    free (buf);
	    buf = NULL;
	  }
	fclose (file);
      }

      /* Send the message.  */
      mailer_send_message (mailer, msg);
      message_destroy (&msg, NULL);
      mailer_destroy (&mailer);
      remove (filename);
      return 0;
    }

  remove (filename);
  return 1;
}
