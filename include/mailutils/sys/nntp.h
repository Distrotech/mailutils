/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2004 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_NNTP_H
#define _MAILUTILS_SYS_NNTP_H

#include <sys/types.h>
#include <mailutils/nntp.h>
#include <mailutils/errno.h>

#ifdef DMALLOC
# include <dmalloc.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum mu_nntp_state
  {
    MU_NNTP_NO_STATE,
    MU_NNTP_CONNECT, MU_NNTP_GREETINGS,
    MU_NNTP_ARTICLE, MU_NNTP_ARTICLE_ACK, MU_NNTP_ARTICLE_RX,
    MU_NNTP_HEAD,    MU_NNTP_HEAD_ACK,    MU_NNTP_HEAD_RX,
    MU_NNTP_BODY,    MU_NNTP_BODY_ACK, MU_NNTP_BODY_RX,
    MU_NNTP_STAT,    MU_NNTP_STAT_ACK,
    MU_NNTP_STLS,    MU_NNTP_STLS_ACK, MU_NNTP_STLS_CONNECT,
    MU_NNTP_DONE,    MU_NNTP_UNKNOWN,  MU_NNTP_ERROR
  };

/* Structure holding the data necessary to do proper buffering.  */
struct mu_nntp_work_buf
  {
    char *buf;
    char *ptr;
    char *nl;
    size_t len;
  };

/* Structure to hold things general to nntp connection, like its state, etc ... */
struct _mu_nntp
  {
    /* Working I/O buffer.
       io.buf: Working io buffer
       io.ptr: Points to the end of the buffer, the non consumed chars
       io.nl: Points to the '\n' char in the string
       io.len: Len of io_buf.  */
    struct mu_nntp_work_buf io;

    /* Holds the first line response of the last command, i.e the ACK:
       ack.buf: Buffer for the ack
       ack.ptr: Working pointer, indicate the start of the non consumed chars
       ack.len: Size 512 according to RFC2449.  */
    struct mu_nntp_work_buf ack;
    int acknowledge;

    unsigned timeout;  /* Default is 10 minutes.  */

    mu_debug_t debug; /* debugging trace.  */

    enum mu_nntp_state state;  /* Indicate the state of the running command.  */

    stream_t carrier; /* TCP Connection.  */
  };

extern int  mu_nntp_debug_cmd        (mu_nntp_t);
extern int  mu_nntp_debug_ack        (mu_nntp_t);
extern int  mu_nntp_stream_create    (mu_nntp_t pop3, stream_t *pstream);
extern int  mu_nntp_carrier_is_ready (stream_t carrier, int flag, int timeout);

/* Check for non recoverable error.
   The error is consider not recoverable if not part of the signal set:
   EAGAIN, EINPROGRESS, EINTR.
   For unrecoverable error we reset, by moving the working ptr
   to the begining of the buffer and setting the state to error.
 */
#define MU_NNTP_CHECK_EAGAIN(nntp, status) \
do \
  { \
    if (status != 0) \
      { \
         if (status != EAGAIN && status != EINPROGRESS && status != EINTR) \
           { \
             nntp->io.ptr = nntp->io.buf; \
             nntp->state = MU_NNTP_ERROR; \
           } \
         return status; \
      } \
   }  \
while (0)

/* If error return.
   Check status an reset(see MU_NNTP_CHECK_EAGAIN) the buffer.
  */
#define MU_NNTP_CHECK_ERROR(nntp, status) \
do \
  { \
     if (status != 0) \
       { \
          nntp->io.ptr = nntp->io.buf; \
          nntp->state = MU_NNTP_ERROR; \
          return status; \
       } \
  } \
while (0)

/* Check if we got "2xx". In NNTP protocol and ack of "2xx" means the command was successfull.
 */
#define MU_NNTP_CHECK_OK(nntp) \
do \
  { \
     if (nntp->ack.buf[0] != '2') \
       { \
          nntp->state = MU_NNTP_NO_STATE; \
          return EACCES; \
       } \
  } \
while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_NNTP_H */
