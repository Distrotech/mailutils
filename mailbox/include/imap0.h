/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _IMAP0_H
#define _IMAP0_H

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <folder0.h>
#include <mailbox0.h>
#include <registrar0.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

#define CLEAR_STATE(f_imap) \
 f_imap->id = 0, f_imap->func = NULL, f_imap->state = IMAP_NO_STATE

/* Clear the state and close the stream.  */
#define CHECK_ERROR_CLOSE(folder, f_imap, status) \
do \
  { \
     if (status != 0) \
       { \
          stream_close (folder->stream); \
          CLEAR_STATE (f_imap); \
          return status; \
       } \
  } \
while (0)

/* Clear the state.  */
#define CHECK_ERROR(f_imap, status) \
do \
  { \
     if (status != 0) \
       { \
          CLEAR_STATE (f_imap); \
          return status; \
       } \
  } \
while (0)

/* Clear the state for non recoverable error.  */
#define CHECK_EAGAIN(f_imap, status) \
do \
  { \
    if (status != 0) \
      { \
         if (status != EAGAIN && status != EINPROGRESS && status != EINTR) \
           { \
             CLEAR_STATE (f_imap); \
           } \
         return status; \
      } \
   }  \
while (0)


struct _f_imap;
struct _m_imap;
struct _msg_imap;
typedef struct _f_imap *f_imap_t;
typedef struct _m_imap *m_imap_t;
typedef struct _msg_imap *msg_imap_t;

enum imap_state
{
  IMAP_NO_STATE=0,
  IMAP_AUTH, IMAP_AUTH_DONE,
  IMAP_BODY,
  IMAP_CLOSE, IMAP_CLOSE_ACK,
  IMAP_DELETE, IMAP_DELETE_ACK,
  IMAP_FETCH, IMAP_FETCH_ACK,
  IMAP_GREETINGS,
  IMAP_HEADER,
  IMAP_HEADER_FIELD,
  IMAP_LIST, IMAP_LIST_PARSE, IMAP_LIST_ACK,
  IMAP_LOGIN, IMAP_LOGIN_ACK,
  IMAP_LOGOUT, IMAP_LOGOUT_ACK,
  IMAP_LSUB, IMAP_LSUB_ACK,
  IMAP_MESSAGE,
  IMAP_NOOP, IMAP_NOOP_ACK,
  IMAP_OPEN_CONNECTION,
  IMAP_RENAME, IMAP_RENAME_ACK,
  IMAP_SELECT, IMAP_SELECT_ACK,
  IMAP_STORE, IMAP_STORE_ACK,
  IMAP_SUBSCRIBE, IMAP_SUBSCRIBE_ACK,
  IMAP_UNSUBSCRIBE, IMAP_UNSUBSCRIBE_ACK
};

struct literal_string
{
  char *buffer;
  size_t buflen;
  size_t total;
  msg_imap_t msg_imap;
  enum imap_state type;
  struct folder_list flist;
  size_t nleft;  /* nleft to read in the literal. */
};

struct _f_imap
{
  /* Back pointer.  */
  folder_t folder;
  m_imap_t selected;

  enum imap_state state;
  void *func;
  size_t id;

  size_t seq; /* Sequence number to build a tag.  */
  char *capa; /* Cabilities of the server.  */
  size_t flags;
  struct literal_string callback;

  int isopen;
  /* Buffer I/O  */
  size_t buflen;
  char *buffer;
  char *ptr;
  char *nl;
  off_t offset; /* Dummy, this is use because of the stream buffering.
                   The stream_t maintains and offset and the offset we use must
                   be in sync.  */

  /* Login  */
  char *user;
  char *passwd;

};

struct _m_imap
{
  /* Back pointers.  */
  mailbox_t mailbox;
  f_imap_t f_imap;
  size_t messages_count;
  size_t imessages_count;
  msg_imap_t *imessages;
  size_t recent;
  size_t unseen;
  unsigned long uidvalidity;
  size_t uidnext;
  char *name;
};

struct _msg_imap
{
  /* Back pointers.  */
  message_t message;
  m_imap_t m_imap;
  size_t num;
  size_t part;
  size_t num_parts;
  msg_imap_t *parts;
  msg_imap_t parent;
  int flags;
  size_t uid;

  size_t message_size;
  size_t message_lines;
  size_t body_size;
  size_t body_lines;
  size_t header_size;
  size_t header_lines;
};

int folder_imap_open  __P ((folder_t, int));
int folder_imap_close __P ((folder_t));

int imap_writeline    __P ((f_imap_t,  const char *format, ...));
int imap_write        __P ((f_imap_t));
int imap_send         __P ((f_imap_t));
int imap_parse        __P ((f_imap_t));
int imap_readline     __P ((f_imap_t));
char *section_name      __P ((msg_imap_t));

#ifdef __cplusplus
}
#endif

#endif /* _IMAP0_H */
