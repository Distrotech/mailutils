/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001, 2002, 2003,
   2004 Free Software Foundation, Inc.

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

#define MAX_OPEN_STREAMS 16

/* Notifications ADD_MESG. */
#define DISPATCH_ADD_MSG(mbox,mhd) \
do \
{ \
  int bailing = 0; \
  monitor_unlock (mbox->monitor); \
  if (mbox->observable) \
     bailing = observable_notify (mbox->observable, MU_EVT_MESSAGE_ADD); \
  if (bailing != 0) \
    { \
      if (pcount) \
        *pcount = (mhd)->msg_count; \
      locker_unlock (mbox->locker); \
      return EINTR; \
    } \
  monitor_wrlock (mbox->monitor); \
} while (0);

struct _amd_data;
struct _amd_message
{
  struct _amd_message *next;
  struct _amd_message *prev;

  stream_t stream;          /* Associated file stream */
  off_t body_start;         /* Offset of body start in the message file */
  off_t body_end;           /* Offset of body end (size of file, effectively)*/

  int attr_flags;           /* Attribute flags */
  int deleted;              /* Was the message originally deleted */

  time_t mtime;             /* Time of last modification */
  size_t header_lines;      /* Number of lines in the header part */
  size_t body_lines;        /* Number of lines in the body */

  message_t message; /* Corresponding message_t */
  struct _amd_data *amd;    /* Back pointer.  */
};

struct _amd_data
{
  size_t msg_size;               /* Size of struct _amd_message */
  int (*msg_init_delivery) (struct _amd_data *, struct _amd_message *);
  int (*msg_finish_delivery) (struct _amd_data *, struct _amd_message *);
  void (*msg_free) (struct _amd_message *);
  char *(*msg_file_name) (struct _amd_message *, int deleted);
  int (*scan0)     (mailbox_t mailbox, size_t msgno, size_t *pcount,
		    int do_notify);
  int (*msg_cmp) (struct _amd_message *, struct _amd_message *);
  int (*message_uid) (message_t msg, size_t *puid);
  size_t (*next_uid) (struct _amd_data *mhd);
  
  /* List of messages: */
  size_t msg_count; /* number of messages in the list */
  struct _amd_message *msg_head;  /* First */
  struct _amd_message *msg_tail;  /* Last */

  unsigned long uidvalidity;

  char *name;                    /* Directory name */

  /* Pool of open message streams */
  struct _amd_message *msg_pool[MAX_OPEN_STREAMS];
  int pool_first;    /* Index to the first used entry in msg_pool */
  int pool_last;     /* Index to the first free entry in msg_pool */

  time_t mtime;      /* Time of last modification */

  mailbox_t mailbox; /* Back pointer. */
};


int amd_init_mailbox __P((mailbox_t mailbox, size_t mhd_size,
			  struct _amd_data **pmhd));
void _amd_message_insert __P((struct _amd_data *mhd,
			      struct _amd_message *msg));
int amd_message_stream_open __P((struct _amd_message *mhm));
void amd_message_stream_close __P((struct _amd_message *mhm));
void amd_cleanup (void *arg);
