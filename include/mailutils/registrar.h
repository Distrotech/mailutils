/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_REGISTRAR_H
#define _MAILUTILS_REGISTRAR_H

#include <sys/types.h>

#include <mailutils/url.h>
#include <mailutils/mailbox.h>
#include <mailutils/mailer.h>
#include <mailutils/folder.h>
#include <mailutils/list.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

#ifdef _cplusplus
extern "C" {
#endif

/* Record.  */
struct _record;
typedef struct _record* record_t;

/* Public Interface, to allow static initialization.  */
struct _record
{
  /* Should not be access directly but rather by the stub functions.  */
  const char *scheme;
  int (*_url)     __P ((url_t));
  int (*_mailbox) __P ((mailbox_t));
  int (*_mailer)  __P ((mailer_t));
  int (*_folder)  __P ((folder_t));
  void *data; /* back pointer.  */

  /* Stub functions to override. The defaut is to return the fields.  */
  int (*_is_scheme)   __P ((record_t, const char *));
  int (*_get_url)     __P ((record_t, int (*(*_url)) __P ((url_t))));
  int (*_get_mailbox) __P ((record_t, int (*(*_mailbox)) __P ((mailbox_t))));
  int (*_get_mailer)  __P ((record_t, int (*(*_mailer)) __P ((mailer_t))));
  int (*_get_folder)  __P ((record_t, int (*(*_folder)) __P ((folder_t))));
};

/* Registration.  */
extern int registrar_get_list     __P ((list_t *));
extern int registrar_record       __P ((record_t));
extern int unregistrar_record     __P ((record_t));

/* Scheme.  */
extern int record_is_scheme       __P ((record_t, const char *));
extern int record_set_scheme      __P ((record_t, const char *));
extern int record_set_is_scheme   __P ((record_t, int (*_is_scheme)
					__P ((record_t, const char *))));

/* Url.  */
extern int record_get_url         __P ((record_t, int (*(*)) __P ((url_t))));
extern int record_set_url         __P ((record_t, int (*) __P ((url_t))));
extern int record_set_get_url     __P ((record_t, int (*_get_url)
					__P ((record_t,
					      int (*(*)) __P ((url_t))))));
/*  Mailbox. */
extern int record_get_mailbox     __P ((record_t, int (*(*))
					__P ((mailbox_t))));
extern int record_set_mailbox     __P ((record_t, int (*) __P ((mailbox_t))));
extern int record_set_get_mailbox __P ((record_t, int (*_get_mailbox)
					__P ((record_t,
					      int (*(*)) __P((mailbox_t))))));
/* Mailer.  */
extern int record_get_mailer      __P ((record_t,
					int (*(*)) __P ((mailer_t))));
extern int record_set_mailer      __P ((record_t, int (*) __P ((mailer_t))));
extern int record_set_get_mailer  __P ((record_t, int (*_get_mailer)
				       __P ((record_t,
					     int (*(*)) __P ((mailer_t))))));
/* Folder.  */
extern int record_get_folder      __P ((record_t,
					int (*(*)) __P ((folder_t))));
extern int record_set_folder      __P ((record_t, int (*) __P ((folder_t))));
extern int record_set_get_folder  __P ((record_t, int (*_get_folder)
					__P ((record_t,
					      int (*(*)) __P ((folder_t))))));

/* Records provided by the library.  */

/* Remote Folder "imap://"  */
extern record_t imap_record;
/* Remote Mailbox POP3, pop://  */
extern record_t pop_record;

/* Local Mailbox Unix Mailbox, "mbox:"  */
extern record_t mbox_record;
/* Local Folder/Mailbox, "file:"  */
extern record_t file_record;
/* Local Folder/Mailbox, /  */
extern record_t path_record;

/* SMTP mailer, "smtp://"  */
extern record_t smtp_record;
/* Sendmail, "sendmail:"  */
extern record_t sendmail_record;

#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_REGISTRAR_H */
