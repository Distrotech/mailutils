/* copyright and license info go here */

#include <mailbox.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/spool/mail"
#endif

int
main (int argc, char **argv)
{
  char *foo, bar[80];
  int i = 0;
  char c = 0;
  char *mboxname;
  mailbox *mbox;

  if (argc != 1)
    {
      fprintf (stderr, "Usage: %s\n", argv[0]);
      return 1;
    }

  mboxname = getenv ("MAIL");
  if (mboxname == NULL)
    {
      foo = getenv ("LOGNAME");
      mboxname = malloc ((strlen(foo) + strlen(_PATH_MAILDIR) + 2) * sizeof(char));
      strcpy (mboxname, _PATH_MAILDIR "/");
      strcat (mboxname, foo);
      free (foo);
    }

  mbox = mbox_open (mboxname);
  printf ("Number of messages: %d\n", mbox->messages);
  while (1)
    {
      printf ("& ");
      fflush (stdout);
      fgets (bar, 80, stdin);
      c = bar[0];
      if (c == 'd' || c == 'D' || c == 'b' || c == 'B' || c == 'h' ||
	  c == 'H' || c == 'r' || c == 'R')
	{
	  printf ("# ");
	  fgets (bar, 80, stdin);
	}
      switch (c)
	{
	case 'q':
	case 'Q':
	  mbox_close (mbox);
	  return 0;
	  break;
	case 'r':
	case 'R':
 	  foo = mbox_get_header (mbox, atoi (bar) - 1);
          printf ("%s\n", foo);
	  free (foo);
	  foo = mbox_get_body (mbox, atoi (bar) - 1);
	  printf ("%s", foo);
	  free (foo);
	  break;
	case 'h':
	case 'H':
	  foo = mbox_get_header (mbox, atoi (bar) - 1);
	  printf ("%s", foo);
	  free (foo);
	  break;
	case 'b':
	case 'B':
	  foo = mbox_get_body (mbox, atoi (bar) - 1);
          printf ("%s", foo);
          free (foo);
          break;
	case 'd':
	case 'D':
	  mbox_delete (mbox, atoi (bar) - 1);
	  break;
	case 'x':
	case 'X':
	  mbox_expunge (mbox);
	  break;
	default:
	  break;
	}
    }
}
