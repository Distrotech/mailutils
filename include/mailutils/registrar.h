/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA  */

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
  int (*_url)     __PMT ((url_t));
  int (*_mailbox) __PMT ((mailbox_t));
  int (*_mailer)  __PMT ((mailer_t));
  int (*_folder)  __PMT ((folder_t));
  void *data; /* back pointer.  */

  /* Stub functions to override. The defaut is to return the fields.  */
  int (*_is_scheme)   __PMT ((record_t, const char *));
  int (*_get_url)     __PMT ((record_t, int (*(*_url)) __PMT ((url_t))));
  int (*_get_mailbox) __PMT ((record_t, int (*(*_mailbox)) __PMT ((mailbox_t))));
  int (*_get_mailer)  __PMT ((record_t, int (*(*_mailer)) __PMT ((mailer_t))));
  int (*_get_folder)  __PMT ((record_t, int (*(*_folder)) __PMT ((folder_t))));
};

/* Registration.  */
extern int registrar_get_list     __P ((list_t *));
extern int registrar_record       __P ((record_t));
extern int unregistrar_record     __P ((record_t));

/* Scheme.  */
extern int record_is_scheme       __P ((record_t, const char *));
extern int record_set_scheme      __P ((record_t, const char *));
extern int record_set_is_scheme   __P ((record_t, int (*_is_scheme)
					__PMT ((record_t, const char *))));

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
/* Remote newsgroup NNTP, nntp://  */
extern record_t nntp_record;

/* Local Mailbox Unix Mailbox, "mbox:"  */
extern record_t mbox_record;
/* Local Folder/Mailbox, /  */
extern record_t path_record;
/* Local MH, "mh:" */
extern record_t mh_record;
/* Maildir, "maildir:" */
extern record_t maildir_record;

/* SMTP mailer, "smtp://"  */
extern record_t smtp_record;
/* Sendmail, "sendmail:"  */
extern record_t sendmail_record;

#define mu_register_all_mbox_formats() do {\
  list_t bookie = 0;\
  registrar_get_list (&bookie);\
  list_append (bookie, path_record);\
  list_append (bookie, mbox_record);\
  list_append (bookie, pop_record);\
  list_append (bookie, imap_record);\
  list_append (bookie, mh_record);\
  list_append (bookie, maildir_record);\
} while (0)

#define mu_register_local_mbox_formats() do {\
  list_t bookie = 0;\
  registrar_get_list (&bookie);\
  list_append (bookie, path_record);\
  list_append (bookie, mbox_record);\
  list_append (bookie, mh_record);\
  list_append (bookie, maildir_record);\
} while (0)

#define mu_register_remote_mbox_formats() do {\
  list_t bookie = 0;\
  registrar_get_list (&bookie);\
  list_append (bookie, pop_record);\
  list_append (bookie, imap_record);\
  list_append (bookie, nntp_record);\
} while (0)

#define mu_register_all_mailer_formats() do {\
  list_t bookie = 0;\
  registrar_get_list (&bookie);\
  list_append (bookie, sendmail_record);\
  list_append (bookie, smtp_record);\
} while (0)

#define mu_register_all_formats() do {\
  mu_register_all_mbox_formats ();\
  mu_register_all_mailer_formats ();\
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_REGISTRAR_H */
