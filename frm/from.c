/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2005 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

#include <frm.h>

int count_only;
char *sender_option;
char *mailbox_name;

const char *program_version = "from (" PACKAGE_STRING ")";
static char doc[] = N_("GNU from -- display from and subject");

static struct argp_option options[] = {
  {"count",  'c', NULL,   0, N_("Just print a count of messages and exit")},
  {"sender", 's', N_("ADDRESS"), 0,
   N_("Print only mail from addresses containing the supplied string") },
  {"file",   'f', N_("FILE"), 0,
   N_("Read mail from FILE") },
  {"debug",  'd', NULL,   0, N_("Enable debugging output"), 0},
  {0, 0, 0, 0}
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'c':
      count_only = 1;
      break;
      
    case 's':
      sender_option = arg;
      break;
      
    case 'f':
      mailbox_name = arg;
      break;
      
    case 'd':
      frm_debug++;
      break;

    default: 
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options,
  parse_opt,
  N_("[OPTIONS] [USER]"),
  doc,
};

static const char *capa[] = {
  "common",
  "license",
  "mailbox",
#ifdef WITH_TLS
  "tls",
#endif
  NULL
};

static int
from_select (size_t index, message_t msg)
{
  if (count_only)
    return 0;

  if (sender_option)
    {
      int rc = 0;
      header_t hdr = NULL;
      char *sender;
      message_get_header (msg, &hdr);

      if (header_aget_value_unfold (hdr, MU_HEADER_FROM, &sender) == 0)
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
  int c;
  int status = 0;
  size_t total;
  
  /* Native Language Support */
  mu_init_nls ();

  /* register the formats.  */
  mu_register_all_mbox_formats ();

  mu_argp_init (program_version, NULL);
#ifdef WITH_TLS
  mu_tls_init_client_argp ();
#endif
  mu_argp_parse (&argp, &argc, &argv, 0, capa, &c, NULL);

  if (argc - c > 1)
    {
      mu_error (_("Too many arguments"));
      exit (1);
    }
  else if (argc - c > 0)
    {
      if (mailbox_name)
	{
	  mu_error (_("Both --from option and user name are specified"));
	  exit (1);
	}

      mailbox_name = xmalloc (strlen (argv[c]) + 2);
      mailbox_name[0] = '%';
      strcpy (mailbox_name + 1, argv[c]);
    }

  init_output (0);
  
  frm_scan (mailbox_name, from_select, &total);

  if (count_only)
    {
      printf (ngettext ("There is %lu message in your incoming mailbox.\n",
			"There are %lu messages in your incoming mailbox.\n",
			total),
	      (unsigned long) total);
    }
  return 0;
}
  