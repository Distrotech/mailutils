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

#include "mail.h"


static char *
source_readline(void *closure, int cont)
{
  FILE *fp = closure;
  size_t s = 0;
  char *buf = NULL;
  
  (void)cont; /*unused*/
  if (getline (&buf, &s, fp) >= 0)
    {
      int len = strlen (buf);
      if (buf[len-1] == '\n')
	buf[len-1] = '\0';
      return buf;
    }
  return NULL;
}
  
/*
 * so[urce] file
 */

int
mail_source (int argc, char **argv)
{
  FILE *fp;
  int save_term;
  
  if (argc != 2)
    {
      fprintf(ofile, "source requires an argument\n");
      return 1;
    }
  
  fp = fopen (argv[1], "r");
  if (!fp)
    {
      fprintf(ofile, "can't open `%s': %s\n", argv[1], strerror(errno));
      return 1;
    }

  save_term = mail_is_terminal();
  mail_set_is_terminal(0);
  mail_mainloop(source_readline, fp, 0);
  mail_set_is_terminal(save_term);
  fclose (fp);
  return 0;
}
