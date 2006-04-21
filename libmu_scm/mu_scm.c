/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2005, 2006 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library; if not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301 USA */

#include "mu_scm.h"

#ifndef _PATH_SENDMAIL
# define _PATH_SENDMAIL "/usr/lib/sendmail"
#endif

void
mu_scm_error (const char *func_name, int status,
	      const char *fmt, SCM args)
{
  scm_error_scm (scm_from_locale_symbol ("mailutils-error"),
		 func_name ? scm_from_locale_string (func_name) : SCM_BOOL_F,
		 scm_makfrom0str (fmt),
		 args,
		 scm_list_1 (scm_from_int (status)));
}

SCM
mu_scm_makenum (unsigned long val)
#ifndef HAVE_SCM_LONG2NUM
{
  if (SCM_FIXABLE ((long) val))
    return scm_from_int (val);

#ifdef SCM_BIGDIG
  return scm_long2big (val);
#else /* SCM_BIGDIG */
  return scm_make_real ((double) val);
#endif /* SCM_BIGDIG */
}
#else
{
  return scm_long2num (val);
}
#endif

void
mu_set_variable (const char *name, SCM value)
{
  scm_c_define (name, value);
}

SCM _mu_scm_package_string; /* STRING: PACKAGE_STRING */
SCM _mu_scm_package;        /* STRING: PACKAGE */
SCM _mu_scm_version;        /* STRING: VERSION */
SCM _mu_scm_mailer;         /* STRING: Default mailer path. */
SCM _mu_scm_debug;          /* NUM: Default debug level. */

struct format_record {
  char *name;
  mu_record_t *record;
};

static struct format_record format_table[] = {
  { "mbox", &mu_mbox_record },
  { "mh",   &mu_mh_record },
  { "maildir", &mu_maildir_record },
  { "pop",  &mu_pop_record },
  { "imap", &mu_imap_record },
  { "sendmail", &mu_sendmail_record },
  { "smtp", &mu_smtp_record },
  { NULL, NULL },
};

static mu_record_t *
find_format (const struct format_record *table, const char *name)
{
  for (; table->name; table++)
    if (strcmp (table->name, name) == 0)
      break;
  return table->record;
}
		
static int
register_format (const char *name)
{
  int status = 0;
  
  if (!name)
    {
      struct format_record *table;
      for (table = format_table; table->name; table++)
	mu_registrar_record (*table->record);
    }
  else
    {
      mu_record_t *record = find_format (format_table, name);
      if (record)
	status = mu_registrar_record (*record);
      else
	status = EINVAL;
    }
  return status;
}
    

SCM_DEFINE (scm_mu_register_format, "mu-register-format", 0, 0, 1,
	    (SCM REST),
"Registers desired mailutils formats. Takes any number of arguments.\n"
"Allowed arguments are:\n"
"  \"mbox\"       Regular UNIX mbox format\n"
"  \"mh\"         MH mailbox format\n"
"  \"pop\"        POP mailbox format\n"
"  \"imap\"       IMAP mailbox format\n"
"  \"sendmail\"   sendmail mailer\n"
"  \"smtp\"       smtp mailer\n"
"\n"
"If called without arguments, registers all available formats\n")
#define FUNC_NAME s_scm_mu_register_format
{
  int status;

  if (REST == SCM_EOL)
    {
      status = register_format (NULL);
      if (status)
	mu_scm_error (FUNC_NAME, status,
		      "Cannot register formats",
		      SCM_BOOL_F);
    }
  else
    {
      for (; REST != SCM_EOL; REST = SCM_CDR (REST))
	{
	  SCM scm = SCM_CAR (REST);
	  SCM_ASSERT (scm_is_string (scm), scm, SCM_ARGn, FUNC_NAME);
	  status = register_format (scm_i_string_chars (scm));
	  if (status)
	    mu_scm_error (FUNC_NAME, status,
			  "Cannot register format ~A",
			  scm_list_1 (scm));
	}
    }
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_mu_strerror, "mu-strerror", 1, 0, 0,
	    (SCM ERR),
"Return the error message corresponding to ERR, which must be\n"
"an integer value.\n")
#define FUNC_NAME s_scm_mu_strerror
{
  SCM_ASSERT (scm_is_integer (ERR), ERR, SCM_ARG1, FUNC_NAME);
  return scm_makfrom0str (mu_strerror (scm_to_int (ERR)));
}
#undef FUNC_NAME

static struct
{
  char *name;
  int value;
} attr_kw[] = {
  { "MU-ATTRIBUTE-ANSWERED",  MU_ATTRIBUTE_ANSWERED },
  { "MU-ATTRIBUTE-FLAGGED",   MU_ATTRIBUTE_FLAGGED },
  { "MU-ATTRIBUTE-DELETED",   MU_ATTRIBUTE_DELETED }, 
  { "MU-ATTRIBUTE-DRAFT",     MU_ATTRIBUTE_DRAFT },   
  { "MU-ATTRIBUTE-SEEN",      MU_ATTRIBUTE_SEEN },    
  { "MU-ATTRIBUTE-READ",      MU_ATTRIBUTE_READ },    
  { "MU-ATTRIBUTE-MODIFIED",  MU_ATTRIBUTE_MODIFIED },
  { "MU-ATTRIBUTE-RECENT",    MU_ATTRIBUTE_RECENT },
  { NULL, 0 }
};

/* Initialize the library */
void
mu_scm_init ()
{
  int i;

  _mu_scm_mailer = scm_makfrom0str ("sendmail:" _PATH_SENDMAIL);
  mu_set_variable ("mu-mailer", _mu_scm_mailer);

  _mu_scm_debug = mu_scm_makenum(0);
  mu_set_variable ("mu-debug", _mu_scm_debug);

  _mu_scm_package = scm_makfrom0str (PACKAGE);
  mu_set_variable ("mu-package", _mu_scm_package);

  _mu_scm_version = scm_makfrom0str (VERSION);
  mu_set_variable ("mu-version", _mu_scm_version);

  _mu_scm_package_string = scm_makfrom0str (PACKAGE_STRING);
  mu_set_variable ("mu-package-string", _mu_scm_package_string);

  /* Create MU- attribute names */
  for (i = 0; attr_kw[i].name; i++)
    scm_c_define(attr_kw[i].name, scm_from_int(attr_kw[i].value));
  
  mu_scm_mutil_init ();
  mu_scm_mailbox_init ();
  mu_scm_message_init ();
  mu_scm_address_init ();
  mu_scm_body_init ();
  mu_scm_logger_init ();
  mu_scm_port_init ();
  mu_scm_mime_init ();
#include "mu_scm.x"

  mu_registrar_record (mu_path_record);
}
