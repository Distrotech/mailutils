/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012, 2014-2016 Free Software Foundation, Inc.

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

#include <mh.h>

static char prog_doc[] = N_("Print numbers of messages in GNU MH sequence");
static char args_doc[] = N_("[SEQUENCE]");

static int uid_option = 1;

static struct mu_option options[] = {
  { "uids",   0, NULL, MU_OPTION_DEFAULT,
    N_("show message UIDs (default)"),
    mu_c_int, &uid_option, NULL, "1" },
  { "numbers", 0, NULL, MU_OPTION_DEFAULT,
    N_("show message numbers"),
    mu_c_int, &uid_option, NULL, "0" },
  MU_OPTION_END
};

static int
_print_number (size_t n, void *data)
{
  printf ("%lu\n", (unsigned long) n);
  return 0;
}

int
main (int argc, char **argv)
{
  mu_mailbox_t mbox;
  mu_msgset_t msgset;
    
  mh_getopt (&argc, &argv, options, MH_GETOPT_DEFAULT_FOLDER,
	     args_doc, prog_doc, NULL);

  mbox = mh_open_folder (mh_current_folder (), MU_STREAM_READ);

  mh_msgset_parse (&msgset, mbox, argc, argv, "cur");
  if (uid_option)
    mu_msgset_foreach_msguid (msgset, _print_number, NULL);
  else
    mu_msgset_foreach_msgno (msgset, _print_number, NULL);
  return 0;
}

  
