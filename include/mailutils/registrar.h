/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILUTILS_REGISTRAR_H
#define _MAILUTILS_REGISTRAR_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

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
  int (*_get_url)     __P ((record_t, int (*(*_url)) __PMT ((url_t))));
  int (*_get_mailbox) __P ((record_t, int (*(*_mailbox)) __PMT ((mailbox_t))));
  int (*_get_mailer)  __P ((record_t, int (*(*_mailer)) __PMT ((mailer_t))));
  int (*_get_folder)  __P ((record_t, int (*(*_folder)) __PMT ((folder_t))));
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
extern int record_get_url         __P ((record_t, int (*(*)) __PMT ((url_t))));
extern int record_set_url         __P ((record_t, int (*) __PMT ((url_t))));
extern int record_set_get_url     __P ((record_t, int (*_get_url)
					__PMT ((record_t,
					      int (*(*)) __PMT ((url_t))))));
/*  Mailbox. */
extern int record_get_mailbox     __P ((record_t, int (*(*))
					__PMT ((mailbox_t))));
extern int record_set_mailbox     __P ((record_t, int (*) __PMT ((mailbox_t))));
extern int record_set_get_mailbox __P ((record_t, int (*_get_mailbox)
					__PMT ((record_t,
					      int (*(*)) __PMT((mailbox_t))))));
/* Mailer.  */
extern int record_get_mailer      __P ((record_t,
					int (*(*)) __PMT ((mailer_t))));
extern int record_set_mailer      __P ((record_t, int (*) __PMT ((mailer_t))));
extern int record_set_get_mailer  __P ((record_t, int (*_get_mailer)
				       __PMT ((record_t,
					     int (*(*)) __PMT ((mailer_t))))));
/* Folder.  */
extern int record_get_folder      __P ((record_t,
					int (*(*)) __PMT ((folder_t))));
extern int record_set_folder      __P ((record_t, int (*) __PMT ((folder_t))));
extern int record_set_get_folder  __P ((record_t, int (*_get_folder)
					__PMT ((record_t,
					      int (*(*)) __PMT ((folder_t))))));

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
/* Local MH, "mh:" */  
extern record_t mh_record;
  
/* SMTP mailer, "smtp://"  */
extern record_t smtp_record;
/* Sendmail, "sendmail:"  */
extern record_t sendmail_record;

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_REGISTRAR_H */
