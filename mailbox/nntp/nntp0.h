/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004, 2005  Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

struct _nntp_folder;
struct _nntp_mailbox;
struct _nntp_message;
typedef struct _nntp_folder *f_nntp_t;
typedef struct _nntp_mailbox *m_nntp_t;
typedef struct _nntp_message *msg_nntp_t;

extern int _nntp_url_init          (url_t);
extern int _nntp_mailbox_init      (mailbox_t);
extern int _nntp_folder_init       (folder_t);

struct _nntp_folder
{
  /* Refcount.  */
  int isopen;

  /* Back pointer.  */
  folder_t folder;

  /* Selected newsgroup.  */
  m_nntp_t selected;

  /* NNTP object.  */
  mu_nntp_t nntp;
};

struct _nntp_mailbox
{
  char status;

  char *name;

  /* Pointer back to the mailbox/newsgroup. */
  mailbox_t mailbox;

  /* Our nntp folder.  */
  f_nntp_t f_nntp;

  /* Read Messages on the newsgroup.  */
  msg_nntp_t *messages;
  size_t messages_count;

  /* Estimated number of articles in the group.  */
  unsigned long number;
  /* High water mark from "GROUP" command  */
  unsigned long high;
  /* Low water mark from "GROUP" command  */
  unsigned long low;
};

struct _nntp_message
{
  /* Back pointer.  */
  message_t message;

  /* Our nntp folder.  */
  m_nntp_t m_nntp;

  /*  Message id.  */
  char *mid;

  /* mesgno of the post.  */
  unsigned long msgno;

  /* Stream for message.  */
  stream_t mstream;
  /* Stream for body.  */
  stream_t bstream;
  /* Stream for header.  */
  stream_t hstream;
};
