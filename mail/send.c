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
      
  printf ("Function not implemented in %s line %d\n", __FILE__, __LINE__);
  return 1;
}
