/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with GNU Mailutils; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _REGISTRAR0_H
#define _REGISTRAR0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <mailutils/registrar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The pop and imap defines are all wrong now, since they seem intertwined
   with the old url parsing code. Also, "pop://" is not the POP scheme,
   at least not as a scheme is described in the RFCs.

   Perhaps they can be changed?
*/
#define MU_POP_PORT 110
#define MU_POP_SCHEME "pop://"
#define MU_POP_SCHEME_LEN 6
extern int _url_pop_init          __P ((url_t));
extern int _mailbox_pop_init      __P ((mailbox_t));
extern int _folder_pop_init       __P ((folder_t));

#define MU_IMAP_PORT 143
#define MU_IMAP_SCHEME "imap://"
#define MU_IMAP_SCHEME_LEN 7
extern int _url_imap_init         __P ((url_t));
extern int _mailbox_imap_init     __P ((mailbox_t));
extern int _folder_imap_init      __P ((folder_t));

#define MU_MBOX_SCHEME "mbox:"
#define MU_MBOX_SCHEME_LEN 5
extern int _url_mbox_init         __P ((url_t));
extern int _mailbox_mbox_init     __P ((mailbox_t));
extern int _folder_mbox_init      __P ((folder_t));

#define MU_FILE_SCHEME "file:"
#define MU_FILE_SCHEME_LEN 5
extern int _url_file_init         __P ((url_t));
extern int _mailbox_file_init     __P ((mailbox_t));
extern int _folder_file_init      __P ((folder_t));

#define MU_PATH_SCHEME "/"
#define MU_PATH_SCHEME_LEN 1
extern int _url_path_init         __P ((url_t));
extern int _mailbox_path_init     __P ((mailbox_t));
extern int _folder_path_init      __P ((folder_t));

#define MU_SMTP_SCHEME "smtp://"
#define MU_SMTP_SCHEME_LEN 7
#define MU_SMTP_PORT 25
extern int _url_smtp_init         __P ((url_t));
extern int _mailer_smtp_init      __P ((mailer_t));

#define MU_SENDMAIL_SCHEME "sendmail:"
#define MU_SENDMAIL_SCHEME_LEN 9
extern int _url_sendmail_init     __P ((url_t));
extern int _mailer_sendmail_init  __P ((mailer_t));

#define MU_MH_SCHEME "mh:"
#define MU_MH_SCHEME_LEN 3
extern int _url_mh_init     __P ((url_t));
extern int _mailbox_mh_init __P((mailbox_t mailbox));
extern int _folder_mh_init  __P ((folder_t));
  
#ifdef __cplusplus
}
#endif

#endif /* _REGISTRAR0_H */
