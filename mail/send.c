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

#include "mail.h"

/*
 * m[ail] address...
 */

int
mail_send (int argc, char **argv)
{
  char *to = NULL, *cc = NULL, *bcc = NULL, *subj = NULL;
  char *filename = tempnam (getenv ("TMPDIR"), "mu");
  FILE *file = fopen (filename, "w");
  char *buf = NULL;
  size_t n;
  int done = 0;

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
	  FILE *ostdout = stdout;
	  stdout = file;
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
	      printf ("%s", (util_find_env("Sign"))->value);
	      break;
	    case 'a':
	      printf ("%s", (util_find_env("sign"))->value);
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
	      stdout = ostdout;
	      util_do_command ("!%s %s", getenv("EDITOR"), filename);
	      file = fopen (filename, "a");
	      stdout = file;
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
	      stdout = ostdout;
	      if (/* numlines (filename) > */ util_getlines())
		util_do_command ("!%s %s", getenv("PAGER"), filename);
	      else
		/* dump filename */;
	      file = fopen (filename, "a");
	      stdout = file;
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
	      stdout = ostdout;
	      util_do_command ("!%s %s", getenv("VISUAL"), filename);
	      file = fopen (filename, "a");
	      stdout = file;
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
	  stdout = ostdout;
	}
      else
	fprintf (file, "%s", buf);
      fflush (file);
      /* free (buf); */
    }

  file = fopen (filename, "r");
  if (file != NULL)
    {
      /* FIXME: create a mailer here */
      while (getline (&buf, &n, file) >= 0)
	{
	  printf("%s", buf);
	  free (buf);
	  buf = NULL;
	}
      fclose (file);
      return 0;
    }
  
  return 1;
}
