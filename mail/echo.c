/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2001-2002, 2007, 2010-2012 Free Software
   Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#include "mail.h"

/*
 * ec[ho] string ...
 */

static int
echo (char *s)
{
  struct mu_wordsplit ws;
  size_t len = strlen (s);
  int rc = 1;

  /* FIXME: This logic is flawed */
  if (len > 0 && s[len - 1] == '\\')
    {
      if (len == 1)
	return 0;
      if (s[len-2] != '\\')
	{
	  --len;
	  rc = 0;
	}
    }
      
      
  if (mu_wordsplit_len (s, len, &ws,
			MU_WRDSF_NOSPLIT |
			MU_WRDSF_NOCMD | MU_WRDSF_NOVAR | MU_WRDSF_QUOTE))
    {
      mu_error (_("cannot split `%s': %s"), s,
		mu_wordsplit_strerror (&ws));
    }
  else
    {
      mu_printf ("%s", ws.ws_wordv[0]);
      mu_wordsplit_free (&ws);
    }
  return rc;
}

int
mail_echo (int argc, char **argv)
{
  if (argc > 1)
    {
      int i = 0;
      
      for (i = 1; i < argc; i++)
	{
	  if (echo (argv[i]))
	    mu_printf (" ");
	}
      mu_printf ("\n");
    }
  return 0;
}

