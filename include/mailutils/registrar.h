/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2004, 2005 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_REGISTRAR_H
#define _MAILUTILS_REGISTRAR_H

#include <mailutils/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public Interface, to allow static initialization.  */
struct _record
{
  int priority;    /* Higher priority records are scanned first */
  const char *scheme;
  int (*_url) (url_t);
  int (*_mailbox) (mailbox_t);
  int (*_mailer) (mailer_t);
  int (*_folder) (folder_t);
  void *data; /* back pointer.  */

  /* Stub functions to override. The default is to return the fields.  */
  int (*_is_scheme) (record_t, const char *, int);
  int (*_get_url) (record_t, int (*(*_url)) (url_t));
  int (*_get_mailbox) (record_t, int (*(*_mailbox)) (mailbox_t));
  int (*_get_mailer) (record_t, int (*(*_mailer)) (mailer_t));
  int (*_get_folder) (record_t, int (*(*_folder)) (folder_t));
};

/* Registration.  */
extern int registrar_get_iterator (iterator_t *);
extern int registrar_get_list (list_t *) __attribute__ ((deprecated));
  
extern int registrar_lookup (const char *name, record_t *precord, int flags);
extern int registrar_record       (record_t);
extern int unregistrar_record     (record_t);

/* Scheme.  */
extern int record_is_scheme       (record_t, const char *, int flags);
extern int record_set_scheme      (record_t, const char *);
extern int record_set_is_scheme   (record_t, 
                      int (*_is_scheme) (record_t, const char *, int));

/* Url.  */
extern int record_get_url         (record_t, int (*(*)) (url_t));
extern int record_set_url         (record_t, int (*) (url_t));
extern int record_set_get_url     (record_t, 
                      int (*_get_url) (record_t, int (*(*)) (url_t)));
/*  Mailbox. */
extern int record_get_mailbox     (record_t, int (*(*)) (mailbox_t));
extern int record_set_mailbox     (record_t, int (*) (mailbox_t));
extern int record_set_get_mailbox (record_t, 
                 int (*_get_mailbox) (record_t, int (*(*)) (mailbox_t)));
/* Mailer.  */
extern int record_get_mailer      (record_t,
				   int (*(*)) (mailer_t));
extern int record_set_mailer      (record_t, int (*) (mailer_t));
extern int record_set_get_mailer  (record_t, 
        int (*_get_mailer) (record_t, int (*(*)) (mailer_t)));
/* Folder.  */
extern int record_get_folder      (record_t, int (*(*)) (folder_t));
extern int record_set_folder      (record_t, int (*) (folder_t));
extern int record_set_get_folder  (record_t, 
         int (*_get_folder) (record_t, int (*(*)) (folder_t)));

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

#define MU_IMAP_PRIO       100
#define MU_POP_PRIO        200
#define MU_MBOX_PRIO       300 
#define MU_MH_PRIO         400
#define MU_MAILDIR_PRIO    500 
#define MU_NNTP_PRIO       600
#define MU_PATH_PRIO       1000

#define MU_SMTP_PRIO       10000
#define MU_SENDMAIL_PRIO   10000
  
/* SMTP mailer, "smtp://"  */
extern record_t smtp_record;
/* Sendmail, "sendmail:"  */
extern record_t sendmail_record;

#define mu_register_all_mbox_formats() do {\
  registrar_record (path_record);\
  registrar_record (mbox_record);\
  registrar_record (pop_record);\
  registrar_record (imap_record);\
  registrar_record (mh_record);\
  registrar_record (maildir_record);\
} while (0)

#define mu_register_local_mbox_formats() do {\
  registrar_record (path_record);\
  registrar_record (mbox_record);\
  registrar_record (mh_record);\
  registrar_record (maildir_record);\
} while (0)

#define mu_register_remote_mbox_formats() do {\
  registrar_record (pop_record);\
  registrar_record (imap_record);\
  registrar_record (nntp_record);\
} while (0)

#define mu_register_all_mailer_formats() do {\
  registrar_record (sendmail_record);\
  registrar_record (smtp_record);\
} while (0)

#define mu_register_extra_formats() do {\
  registrar_record (nntp_record);\
} while (0)

#define mu_register_all_formats() do {\
  mu_register_all_mbox_formats ();\
  mu_register_all_mailer_formats ();\
  mu_register_extra_formats();\
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_REGISTRAR_H */
