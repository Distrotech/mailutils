/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_MBOX_H
#define _MAILUTILS_SYS_MBOX_H

#include <time.h>
#include <mailutils/monitor.h>
#include <mailutils/locker.h>
#include <mailutils/mbox.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mbox_message* mbox_message_t;

/* Below are the headers field-names that we are caching for speed, it is
   more or less the list of headers in ENVELOPE command from IMAP.  */
#define HDRSIZE        15

const char *fhdr_table[] =
{
#define H_BCC                       0
  "Bcc",
#define H_CC                        1
  "Cc",
#define H_CONTENT_LANGUAGE          2
  "Content-Language",
#define H_CONTENT_TRANSFER_ENCODING 3
  "Content-Transfer-Encoding",
#define H_CONTENT_TYPE              4
  "Content-Type",
#define H_DATE                      5
  "Date",
#define H_FROM                      6
  "From",
#define H_IN_REPLY_TO               7
  "In-Reply-To",
#define H_MESSAGE_ID                8
  "Message-ID",
#define H_REFERENCE                 9
  "Reply-To",
#define H_REPLY_TO                  10
  "Reply-To",
#define H_SENDER                    11
  "Sender",
#define H_SUBJECT                   12
  "Subject",
#define H_TO                        13
  "To",
#define H_X_UIDL                    14
  "X-UIDL"
};

/* Keep the file positions of where the headers and bodies start and end.
   attr_flags is the "Status:" message.  */
struct _mbox_message
{
  /* Offset of the messages in the mailbox.  */
  off_t header_from;
  off_t header_from_end;
  off_t body;
  off_t body_end;

  /* Fast header retrieve, we save here the most common headers. This will
     speed the header search.  The entire headers are copied, when modified,
     by the header_t object, we do not have to worry about updating them.  */
  char *fhdr[HDRSIZE];

  size_t uid; /* IMAP uid.  */

  int attr_flags; /* The attr_flags contains the "Status:" attribute  */

  size_t header_lines;
  size_t body_lines;

  message_t message; /* A message attach to it.  */
  mbox_t mud; /* Back pointer.  */
};

/* The umessages is an array of pointers that contains umessages_count of
   mbox_message_t*; umessages[umessages_count].  We do it this way because
   realloc() can move everything to a new memory region and invalidate all
   the pointers.  Thanks to <Dave Inglis> for pointing this out.  The
   messages_count is the count number of messages parsed so far.  */
struct _mbox
{
  mbox_message_t *umessages; /* Array.  */
  size_t umessages_count; /* How big is the umessages[].  */
  size_t messages_count; /* How many valid entry in umessages[].  */

  stream_t stream;
  off_t size; /* Size of the mailbox.  */
  time_t mtime; /* Modified time.  */
  unsigned long uidvalidity;
  size_t uidnext;
  char *filename;

  /* The variables below are use to hold the state when appending messages.  */
  enum mbox_state
  {
    MBOX_NO_STATE = 0,
    MBOX_STATE_APPEND_SENDER, MBOX_STATE_APPEND_DATE, MBOX_STATE_APPEND_HEADER,
    MBOX_STATE_APPEND_ATTRIBUTE, MBOX_STATE_APPEND_UID, MBOX_STATE_APPEND_BODY,
    MBOX_STATE_APPEND_MESSAGE
  } state ;
  char *sender;
  char *to;
  char *date;
  off_t off;
  monitor_t  lock;

  mu_debug_t debug;
  locker_t  locker;
  observable_t observable;
};

/* Moro(?)ic kluge.  */
#define MBOX_DEBUG0(mbox, type, format) \
if (mbox->debug) mu_debug_print (mbox->debug, type, format)
#define MBOX_DEBUG1(mbox, type, format, arg1) \
if (mbox->debug) mu_debug_print (mbox->debug, type, format, arg1)
#define MBOX_DEBUG2(mbox, type, format, arg1, arg2) \
if (mbox->debug) mu_debug_print (mbox->debug, type, format, arg1, arg2)
#define MBOX_DEBUG3(mbox, type, format, arg1, arg2, arg3) \
if (mbox->debug) mu_debug_print (mbox->debug, type, format, arg1, arg2, arg3)
#define MBOX_DEBUG4(mbox, type, format, arg1, arg2, arg3, arg4) \
if (mbox->debug) mu_debug_print (mbox->debug, type, format, arg1, arg2, arg3, arg4)

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_MBOX_H */
