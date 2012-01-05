/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2007, 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see
   <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <mailutils/nls.h>
#include <mailutils/version.h>
#include <mailutils/cstr.h>
#include <mailutils/stream.h>
#include <mailutils/stdstream.h>
#include <mailutils/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <confpaths.h>

static struct mu_conf_option mu_conf_option[] = {
  { "VERSION=" VERSION, N_("Version of this package") },
  { "SYSCONFDIR=" SYSCONFDIR, N_("System configuration directory") },
  { "MAILSPOOLDIR=" MU_PATH_MAILDIR, N_("Default mail spool directory") },
  { "SCHEME=" MU_DEFAULT_SCHEME, N_("Default mailbox type") },
  { "LOG_FACILITY=" LOG_FACILITY_STRING, N_("Default syslog facility") },
#ifdef MAILUTILS_IPV6
  { "IPV6", N_("IPv6 support") },
#endif
#ifdef USE_LIBPAM
  { "USE_LIBPAM", N_("PAM support") },
#endif
#ifdef HAVE_LIBLTDL
  { "HAVE_LIBLTDL", N_("a portable `dlopen' wrapper library") },
#endif
#ifdef WITH_BDB2
  { "WITH_BDB2", N_("Berkeley DB v. 2") },
#endif
#ifdef WITH_BDB
  { "WITH_BDB", N_("Berkeley DB") },
#endif
#ifdef WITH_NDBM
  { "WITH_NDBM", },
#endif
#ifdef WITH_GDBM
  { "WITH_GDBM", N_("GNU DBM") },
#endif
#ifdef WITH_TOKYOCABINET
  { "WITH_TOKYOCABINET", N_("Tokyo Cabinet DBM") },
#endif
#ifdef WITH_KYOTOCABINET
  { "WITH_KYOTOCABINET", N_("Kyoto Cabinet DBM") },
#endif
#ifdef WITH_GNUTLS
  { "WITH_GNUTLS", N_("TLS support using GNU TLS") },
#endif
#ifdef WITH_GSASL
  { "WITH_GSASL", N_("SASL support using GNU SASL") },
#endif
#ifdef WITH_GSSAPI
  { "WITH_GSSAPI", N_("GSSAPI support") },
#endif
#ifdef WITH_GUILE
  { "WITH_GUILE", N_("Support for Guile as extension language") },
#endif
#ifdef WITH_PYTHON
  { "WITH_PYTHON", N_("Support for Python as extension language") },
#endif
#ifdef WITH_PTHREAD
  { "WITH_PTHREAD", N_("Support for POSIX threads") },
#endif
#ifdef WITH_READLINE
  { "WITH_READLINE", N_("GNU Readline") },
#endif
#ifdef HAVE_MYSQL
  { "HAVE_MYSQL", N_("MySQL") },
#endif
#ifdef HAVE_PGSQL
  { "HAVE_PGSQL", N_("PostgreSQL") },
#endif
#ifdef WITH_LDAP
  { "WITH_LDAP", },
#endif
#ifdef WITH_LIBWRAP
  { "WITH_LIBWRAP", N_("Support for TCP wrappers") },
#endif
#ifdef ENABLE_VIRTUAL_DOMAINS
  { "ENABLE_VIRTUAL_DOMAINS", N_("Support for virtual mail domains") },
#endif
#ifdef ENABLE_IMAP
  { "ENABLE_IMAP", N_("IMAP4 protocol support") },
#endif
#ifdef ENABLE_POP
  { "ENABLE_POP",  N_("POP3 protocol support") },
#endif
#ifdef ENABLE_MH
  { "ENABLE_MH", N_("MH mail storage support") },
#endif
#ifdef ENABLE_MAILDIR
  { "ENABLE_MAILDIR", N_("Maildir mail storage support") },
#endif
#ifdef ENABLE_SMTP
  { "ENABLE_SMTP", N_("SMTP protocol support") },
#endif
#ifdef ENABLE_SENDMAIL
  { "ENABLE_SENDMAIL", N_("Sendmail command line interface support")},
#endif
#ifdef ENABLE_NNTP
  { "ENABLE_NNTP", N_("NNTP protocol support") },
#endif
#ifdef ENABLE_RADIUS
  { "ENABLE_RADIUS", N_("RADIUS protocol support") },
#endif
#ifdef WITH_INCLUDED_LIBINTL
  { "WITH_INCLUDED_LIBINTL", N_("GNU libintl compiled in") },
#endif
  { NULL }
};

void
mu_format_conf_option (mu_stream_t stream, const struct mu_conf_option *opt,
		       int verbose)
{
  mu_stream_printf (stream, "%s", opt->name);
  if (verbose && opt->descr)
    mu_stream_printf (stream, " \t- %s", _(opt->descr));
  mu_stream_printf (stream, "\n");
}

void
mu_format_options (mu_stream_t stream, int verbose)
{
  int i;
  
  for (i = 0; mu_conf_option[i].name; i++)
    mu_format_conf_option (stream, mu_conf_option + i, verbose);
}

void
mu_print_options ()
{
  if (mu_strout)
    mu_stream_ref (mu_strout);
  else
    {
      int yes = 1;
      int rc = mu_stdio_stream_create (&mu_strout, MU_STDOUT_FD, 0);
      if (rc)
	{
	  fprintf (stderr, "mu_stdio_stream_create(%d): %s\n",
		   MU_STDOUT_FD, mu_strerror (rc));
	  abort ();
	}
      mu_stream_ioctl (mu_strout, MU_IOCTL_FD, MU_IOCTL_FD_SET_BORROW, &yes);  
    }
  mu_format_options (mu_strout, 1);
  mu_stream_unref (mu_strout);
}

const struct mu_conf_option *
mu_check_option (char *name)
{
  int i;
  
  for (i = 0; mu_conf_option[i].name; i++)
    {
      int len;
      char *q, *p = strchr (mu_conf_option[i].name, '=');
      if (p)
	len = p - mu_conf_option[i].name;
      else
	len = strlen (mu_conf_option[i].name);

      if (mu_c_strncasecmp (mu_conf_option[i].name, name, len) == 0)
	return &mu_conf_option[i];
      else if ((q = strchr (mu_conf_option[i].name, '_')) != NULL
	       && mu_c_strncasecmp (q + 1, name,
			       len - (q - mu_conf_option[i].name) - 1) == 0)
	return &mu_conf_option[i];
    }
  return NULL;
}  

