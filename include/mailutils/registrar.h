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

struct mailbox_entry;
typedef struct mailbox_entry* mailbox_entry_t;
struct mailer_entry;
typedef struct mailer_entry* mailer_entry_t;
struct folder_entry;
typedef struct folder_entry* folder_entry_t;
struct _registrar_record;
typedef struct _registrar_record* registrar_record_t;

struct mailbox_entry
{
  int (*_url_init)     __P ((url_t));
  int (*_mailbox_init) __P ((mailbox_t));
};
struct mailer_entry
{
  int (*_url_init)     __P ((url_t));
  int (*_mailer_init)  __P ((mailer_t));
};
struct folder_entry
{
  int (*_url_init)     __P ((url_t));
  int (*_folder_init)  __P ((folder_t));
};

struct _record;
typedef struct _record* record_t;

/* Registration.  */
extern int registrar_get_list   __P ((list_t *));

/* Record.  */
extern int record_create    __P ((record_t *, void *owner));
extern void record_destroy  __P ((record_t *));

extern int record_is_scheme __P ((record_t, const char *));
extern int record_set_scheme __P ((record_t, const char *));
extern int record_set_is_scheme __P ((record_t, int (*_is_scheme)
				      __P ((const char *))));
extern int record_get_mailbox   __P ((record_t, mailbox_entry_t *));
extern int record_set_mailbox   __P ((record_t, mailbox_entry_t));
extern int record_set_get_mailbox __P ((record_t, int (*_get_mailbox)
					__P ((mailbox_entry_t *))));
extern int record_get_mailer     __P ((record_t, mailer_entry_t *));
extern int record_set_mailer     __P ((record_t, mailer_entry_t));
extern int record_set_get_mailer __P ((record_t, int (*_get_mailer)
				       __P ((mailer_entry_t *))));
extern int record_get_folder     __P ((record_t, folder_entry_t *));
extern int record_set_folder     __P ((record_t, folder_entry_t));
extern int record_set_get_folder __P ((record_t, int (*_get_folder)
				       __P ((folder_entry_t *))));

#define MU_POP_PORT 110
#define MU_POP_SCHEME "pop://"
#define MU_POP_SCHEME_LEN 6
extern int url_pop_init  __P ((url_t));
extern mailbox_entry_t pop_entry;
extern record_t pop_record;

#define MU_IMAP_PORT 143
#define MU_IMAP_SCHEME "imap://"
#define MU_IMAP_SCHEME_LEN 7
extern int url_imap_init  __P ((url_t));
extern folder_entry_t imap_entry;
extern record_t imap_record;

#define MU_MBOX_SCHEME "mbox:"
#define MU_MBOX_SCHEME_LEN 5
extern int url_mbox_init __P ((url_t));
extern mailbox_entry_t mbox_entry;
extern record_t mbox_record;

#define MU_FILE_SCHEME "file:"
#define MU_FILE_SCHEME_LEN 5
extern int url_file_init __P ((url_t));
extern mailbox_entry_t file_entry;
extern record_t file_record;

#define MU_PATH_SCHEME "/"
#define MU_PATH_SCHEME_LEN 1
extern int url_path_init __P ((url_t));
extern mailbox_entry_t path_entry;
extern record_t path_record;

#define MU_SMTP_SCHEME "smtp://"
#define MU_SMTP_SCHEME_LEN 7
#define MU_SMTP_PORT 25
extern int url_smtp_init __P ((url_t));
extern mailer_entry_t smtp_entry;
extern record_t smtp_record;

#define MU_SENDMAIL_SCHEME "sendmail:"
#define MU_SENDMAIL_SCHEME_LEN 9
extern int url_sendmail_init __P ((url_t));
extern mailer_entry_t sendmail_entry;
extern record_t sendmail_record;

#ifdef _cplusplus
}
#endif

#endif /* _MAILUTILS_REGISTRAR_H */
