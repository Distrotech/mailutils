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

#include "imap4d.h"

/*
 * argv[2] == mailbox
 * this needs to share code with EXAMINE
 */

int
imap4d_select (int argc, char **argv)
{
  if (argc > 3)
    return TOO_MANY;
  else if (argc < 3)
    return TOO_FEW;

  /* close previous mailbox */
  if ( /* open mailbox (argv[2]) == */ 0)
    {
      char *flags = NULL;
      int num_messages = 0, recent = 0, uid = 0;
      util_out (argv[0], TAG_NONE, "FLAGS %s", flags);
      util_out (argv[0], TAG_NONE, "%d EXISTS", num_messages);
      util_out (argv[0], TAG_NONE, "%d RECENT", recent);
      return util_finish (argc, argv, RESP_OK, NULL, "Complete");
    }
  return util_finish (argc, argv, RESP_NO, NULL, "Couldn't open %s", argv[2]);
}
