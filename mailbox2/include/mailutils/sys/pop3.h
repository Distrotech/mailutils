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


#ifndef _MAILUTILS_SYS_POP3_H
#define _MAILUTILS_SYS_POP3_H

#include <sys/types.h>
#include <mailutils/pop3.h>
#include <mailutils/sys/stream.h>
#include <mailutils/sys/iterator.h>
#include <mailutils/error.h>
#include <mailutils/refcount.h>

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum pop3_state
{
  POP3_NO_STATE,
  POP3_CONNECT, POP3_GREETINGS,
  POP3_APOP, POP3_APOP_ACK,
  POP3_AUTH, POP3_AUTH_ACK,
  POP3_CAPA, POP3_CAPA_ACK, POP3_CAPA_RX,
  POP3_DELE, POP3_DELE_ACK,
  POP3_LIST, POP3_LIST_ACK, POP3_LIST_RX,
  POP3_NOOP, POP3_NOOP_ACK,
  POP3_PASS, POP3_PASS_ACK,
  POP3_QUIT, POP3_QUIT_ACK,
  POP3_RETR, POP3_RETR_ACK, POP3_RETR_RX,
  POP3_RSET, POP3_RSET_ACK,
  POP3_STAT, POP3_STAT_ACK,
  POP3_TOP,  POP3_TOP_ACK,  POP3_TOP_RX,
  POP3_UIDL, POP3_UIDL_ACK, POP3_UIDL_RX,
  POP3_USER, POP3_USER_ACK,
  POP3_DONE, POP3_UNKNOWN,  POP3_ERROR
};

struct p_iterator
{
  struct _iterator base;
  pop3_t pop3;
  mu_refcount_t refcount;
  int done;
  char *item;
};

struct p_stream
{
  struct _stream base;
  pop3_t pop3;
  mu_refcount_t refcount;
  int done;
};

struct work_buf
{
  char *buf;
  char *ptr;
  char *nl;
  size_t len;
  off_t offset;  /* To synchronise with the buffering.  */
};

/* Structure to hold things general to POP3 mailbox, like its state, etc ... */
struct _pop3
{
  /* Working I/O buffer.  */
  /* io.buf: Working io buffer.  */
  /* io.ptr: Points to the end of the buffer, the non consume chars.  */
  /* io.nl: Points to the '\n' char in the string.  */
  /* io.len: Len of io_buf.  */
  /* io.offset;  full the stream_t implementation.  */
  struct work_buf io;

  /* Holds the first line response of the last command, i.e the ACK.  */
  /* ack.buf: Buffer for the ack.  */
  /* ack.ptr: Working pointer.  */
  /* ack.len: Size 512 according to RFC2449.  */
  struct work_buf ack;
  int acknowledge;

  char *timestamp; /* For apop, if supported.  */
  unsigned timeout;  /* Default is 10 minutes.  */

  enum pop3_state state;
  stream_t carrier; /* TCP Connection.  */
  mu_debug_t debug; /* Send the debug info.  */
};

extern int  pop3_iterator_create __P ((pop3_t, iterator_t *));
extern int  pop3_stream_create   __P ((pop3_t, stream_t *));
extern int  pop3_debug_cmd       __P ((pop3_t));
extern int  pop3_debug_ack       __P ((pop3_t));

/* Check for non recoverable error.  */
#define POP3_CHECK_EAGAIN(pop3, status) \
do \
  { \
    if (status != 0) \
      { \
         if (status != MU_ERROR_TRY_AGAIN && status != MU_ERROR_INTERRUPT) \
           { \
             pop3->io.ptr = pop3->io.buf; \
             pop3->state = POP3_ERROR; \
           } \
         return status; \
      } \
   }  \
while (0)

/* If error return.  */
#define POP3_CHECK_ERROR(pop3, status) \
do \
  { \
     if (status != 0) \
       { \
          pop3->io.ptr = pop3->io.buf; \
          pop3->state = POP3_ERROR; \
          return status; \
       } \
  } \
while (0)

/* Check if we got "+OK".  */
#define POP3_CHECK_OK(pop3) \
do \
  { \
     if (strncasecmp (pop3->ack.buf, "+OK", 3) != 0) \
       { \
          pop3->state = POP3_NO_STATE; \
          return MU_ERROR_OPERATION_DENIED; \
       } \
  } \
while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_POP3_H */
