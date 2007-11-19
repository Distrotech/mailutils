/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/nls.h>
#include <stdio.h>
#include <string.h>

char *mu_license_text =
 N_("   GNU Mailutils is free software; you can redistribute it and/or modify\n"
    "   it under the terms of the GNU General Public License as published by\n"
    "   the Free Software Foundation; either version 3 of the License, or\n"
    "   (at your option) any later version.\n"
    "\n"
    "   GNU Mailutils is distributed in the hope that it will be useful,\n"
    "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "   GNU General Public License for more details.\n"
    "\n"
    "   You should have received a copy of the GNU General Public License along\n"
    "   with GNU Mailutils; if not, write to the Free Software Foundation,\n"
    "   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n"
    "\n"
    "\n"
);

static char *mu_conf_option[] = {
  "VERSION=" VERSION,
#ifdef USE_LIBPAM
  "USE_LIBPAM",
#endif
#ifdef HAVE_LIBLTDL
  "HAVE_LIBLTDL",
#endif
#ifdef WITH_BDB2
  "WITH_BDB2",
#endif
#ifdef WITH_NDBM
  "WITH_NDBM",
#endif
#ifdef WITH_OLD_DBM
  "WITH_OLD_DBM",
#endif
#ifdef WITH_GDBM
  "WITH_GDBM",
#endif
#ifdef WITH_GNUTLS
  "WITH_GNUTLS",
#endif
#ifdef WITH_GSASL
  "WITH_GSASL",
#endif
#ifdef WITH_GSSAPI
  "WITH_GSSAPI",
#endif
#ifdef WITH_GUILE
  "WITH_GUILE",
#endif
#ifdef WITH_PTHREAD
  "WITH_PTHREAD",
#endif
#ifdef WITH_READLINE
  "WITH_READLINE",
#endif
#ifdef HAVE_MYSQL
  "HAVE_MYSQL",
#endif
#ifdef HAVE_PGSQL
  "HAVE_PGSQL",
#endif
#ifdef ENABLE_VIRTUAL_DOMAINS
  "ENABLE_VIRTUAL_DOMAINS",
#endif
#ifdef ENABLE_IMAP
  "ENABLE_IMAP",
#endif
#ifdef ENABLE_POP
  "ENABLE_POP", 
#endif
#ifdef ENABLE_MH
  "ENABLE_MH",
#endif
#ifdef ENABLE_MAILDIR
  "ENABLE_MAILDIR",
#endif
#ifdef ENABLE_SMTP
  "ENABLE_SMTP",
#endif
#ifdef ENABLE_SENDMAIL
  "ENABLE_SENDMAIL",
#endif
#ifdef ENABLE_NNTP
  "ENABLE_NNTP",
#endif
#ifdef ENABLE_RADIUS
  "ENABLE_RADIUS",
#endif
#ifdef WITH_INCLUDED_LIBINTL
  "WITH_INCLUDED_LIBINTL",
#endif
  NULL
};

void
mu_fprint_options (FILE *fp)
{
  int i;
  
  for (i = 0; mu_conf_option[i]; i++)
    fprintf (fp, "%s\n", mu_conf_option[i]);
}

void
mu_print_options (FILE *fp)
{
  mu_fprint_options (stdout);
}

const char *
mu_check_option (char *name)
{
  int i;
  
  for (i = 0; mu_conf_option[i]; i++)
    {
      int len;
      char *q, *p = strchr (mu_conf_option[i], '=');
      if (p)
	len = p - mu_conf_option[i];
      else
	len = strlen (mu_conf_option[i]);

      if (strncasecmp (mu_conf_option[i], name, len) == 0) 
	return mu_conf_option[i];
      else if ((q = strchr (mu_conf_option[i], '_')) != NULL
	       && strncasecmp (q + 1, name,
			       len - (q - mu_conf_option[i]) - 1) == 0)
	return mu_conf_option[i];
    }
  return NULL;
}  

