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

#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <mailutils/mailbox.h>

extern mailbox_t mbox;
extern unsigned int total;

static int
addset (int **set, int *n, unsigned val)
{
  int *tmp;
  tmp = realloc (*set, (*n + 1) * sizeof (**set));
  if (tmp == NULL)
    {
      if (*set)
        free (*set);
      *n = 0;
      *set = NULL;
      return ENOMEM;
    }
  *set = tmp;
  (*set)[*n] = val;
  (*n)++;
  return 0;
}

int
msgset (const int argc, char **argv, int **set, int *n)
{
  int i = 0, lc = 0;
  int undelete = 0;
  int *ret = NULL;

  for (i = 0; i < argc; i++)
    {
      /* Last message */
      if (!strcmp (argv[i], "$") || !strcmp (argv[i], "0"))
	{
	  addset (set, n, total);
	}
      else if (!strcmp (argv[i], "*"))
	{
	  /* all messages */
	  for (i = 1; i <= total; i++)
	    addset (set, n, i);
	  i = argc + 1;
	}
      else if (argv[i][0] == '/')
	{
	  /* FIXME: all messages with pattern following / in
	     the subject line, case insensitive */
	  /* This currently appears to be quit b0rked */
	  message_t msg;
	  header_t hdr;
	  char subj[128];
	  int j = 1, k = 0, len2 = 0;
	  int len = strlen (&argv[i][1]);
	  for (j = 1; j <= total; j++)
	    {
	      mailbox_get_message (mbox, j, &msg);
	      message_get_header (msg, &hdr);
	      header_get_value (hdr, MU_HEADER_SUBJECT, subj, 128, NULL);
	      len2 = strlen (subj);
	      for (k = 0; i < strlen (subj); k++)
		{
		  if (len2 - k >= len
		      && !strncasecmp (&argv[i][1], &subj[k], len))
		    {
		      addset (set, n, j);
		      k = 128;
		    }
		}
	    }
	}
      else if (isalpha(argv[i][0]))
	{
	  /* FIXME: all messages from sender argv[i] */
	}
      else if (strchr (argv[i], '-') != NULL)
	{
	  /* message range */
	  int j, x, y;
	  char *arg = strdup (argv[i]);
	  for (j = 0; j < strlen (arg); j++)
	    if (arg[j] == '-')
	      break;
	  arg[j] = '\0';
	  x = strtol (arg, NULL, 10);
	  y = strtol (&(arg[j + 1]), NULL, 10);
	  for (; x <= y; x++)
	    addset (set, n, x);
	  free (arg);
	}
      else
	{
	  /* single message */
	  addset (set, n, strtol (argv[i], NULL, 10));
	}
    }

  return 0;
}
