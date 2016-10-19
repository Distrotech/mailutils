/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005, 2007-2012, 2014-2016 Free Software Foundation,
   Inc.

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

#include <frm.h>

int count_only;
char *sender_option;
char *mailbox_name;

static struct mu_option from_options[] = {
  { "count",  'c', NULL,   MU_OPTION_DEFAULT,
    N_("just print a count of messages and exit"),
    mu_c_bool, &count_only },
  { "sender", 's', N_("ADDRESS"), MU_OPTION_DEFAULT,
    N_("print only mail from addresses containing the supplied string"),
    mu_c_string, &sender_option },
  { "file",   'f', N_("FILE"), MU_OPTION_DEFAULT,
    N_("read mail from FILE"), 
    mu_c_string, &mailbox_name },
  { "debug",  'd', NULL,   MU_OPTION_DEFAULT,
    N_("enable debugging output"),
    mu_c_incr, &frm_debug },
  MU_OPTION_END
}, *options[] = { from_options, NULL };

static struct mu_cli_setup cli = {
  options,
  NULL,
  N_("GNU from -- display from and subject."),
  N_("[OPTIONS] [USER]"),
};

static char *capa[] = {
  "debug",
  "mailbox",
  "locking",
  NULL
};

static int
from_select (size_t index, mu_message_t msg)
{
  if (count_only)
    return 0;

  if (sender_option)
    {
      int rc = 0;
      mu_header_t hdr = NULL;
      char *sender;
      mu_message_get_header (msg, &hdr);

      if (mu_header_aget_value_unfold (hdr, MU_HEADER_FROM, &sender) == 0)
	{
	  if (strstr (sender, sender_option))
	    rc = 1;
	  free (sender);
	}
      
      return rc;
    }
  
  return 1;
}

int
main (int argc, char **argv)
{
  size_t total;
  
  /* Native Language Support */
  MU_APP_INIT_NLS ();

  /* register the formats.  */
  mu_register_all_mbox_formats ();

  mu_auth_register_module (&mu_auth_tls_module);
  mu_cli (argc, argv, &cli, capa, NULL, &argc, &argv);

  if (argc > 1)
    {
      mu_error (_("too many arguments"));
      exit (1);
    }
  else if (argc > 0)
    {
      if (mailbox_name)
	{
	  mu_error (_("both --from option and user name are specified"));
	  exit (1);
	}

      mailbox_name = mu_alloc (strlen (argv[0]) + 2);
      mailbox_name[0] = '%';
      strcpy (mailbox_name + 1, argv[0]);
    }

  init_output (0);
  
  frm_scan (mailbox_name, from_select, &total);

  if (count_only)
    {
      mu_printf (ngettext ("There is %lu message in your incoming mailbox.\n",
			"There are %lu messages in your incoming mailbox.\n",
			total),
	      (unsigned long) total);
    }
  return 0;
}
  
