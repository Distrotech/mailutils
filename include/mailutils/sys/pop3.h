/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 1999-2001, 2003-2004, 2007, 2009, 2011-2012 Free
   Software Foundation, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General
   Public License along with this library.  If not, see 
   <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_POP3_H
#define _MAILUTILS_SYS_POP3_H

#include <sys/types.h>
#include <mailutils/pop3.h>
#include <mailutils/errno.h>
#include <mailutils/cstr.h>

#ifdef __cplusplus
extern "C" {
#endif

enum mu_pop3_state
  {
    MU_POP3_NO_STATE,
    MU_POP3_CONNECT, MU_POP3_GREETINGS,
    MU_POP3_APOP,
    MU_POP3_AUTH,
    MU_POP3_CAPA, MU_POP3_CAPA_RX,
    MU_POP3_DELE,
    MU_POP3_LIST, MU_POP3_LIST_RX,
    MU_POP3_NOOP,
    MU_POP3_PASS,
    MU_POP3_QUIT,
    MU_POP3_RETR, MU_POP3_RETR_RX,
    MU_POP3_RSET,
    MU_POP3_STAT,
    MU_POP3_STLS, MU_POP3_STLS_CONNECT,
    MU_POP3_TOP,  MU_POP3_TOP_RX,
    MU_POP3_UIDL, MU_POP3_UIDL_RX,
    MU_POP3_USER,
    MU_POP3_DONE,
    MU_POP3_UNKNOWN,
    MU_POP3_ERROR
  };

#define MU_POP3_ACK   0x01
#define MU_POP3_TRACE 0x02  
#define MU_POP3_XSCRIPT_MASK(n) (1<<((n)+1))
  
/* Structure to hold things general to POP3 mailbox, like its state, etc ... */
struct _mu_pop3
  {
    int flags;
    
    /* Holds the first line response of the last command, i.e the ACK  */
    char *ackbuf;
    size_t acksize;

    char *rdbuf;
    size_t rdsize;
    
    char *timestamp;    /* For apop, if supported.  */
    unsigned timeout;   /* Default is 10 minutes.  */

    enum mu_pop3_state state;  /* Indicate the state of the running
				  command.  */
    mu_list_t capa;            /* Capabilities. */ 
    mu_stream_t carrier;       /* TCP Connection. */
  };

#define MU_POP3_FSET(p,f) ((p)->flags |= (f))
#define MU_POP3_FISSET(p,f) ((p)->flags & (f))  
#define MU_POP3_FCLR(p,f) ((p)->flags &= ~(f))
  
extern int  mu_pop3_iterator_create (mu_pop3_t pop3, mu_iterator_t *piterator);
extern int  mu_pop3_stream_create (mu_pop3_t pop3, mu_stream_t *pstream);
extern int  mu_pop3_carrier_is_ready (mu_stream_t carrier, int flag,
				      int timeout);

int _mu_pop3_xscript_level (mu_pop3_t pop3, int xlev);

int _mu_pop3_trace_enable (mu_pop3_t pop3);
int _mu_pop3_trace_disable (mu_pop3_t pop3);  

int _mu_pop3_init (mu_pop3_t pop3);
  
/* Check if status indicates an error.
   If the error is recoverable just return the status.
   Otherwise, set the error state and return the status
 */
#define MU_POP3_CHECK_EAGAIN(pop3, status)	\
  do						\
    {						\
      switch (status)				\
	{					\
        case 0:                                 \
	  break;				\
        case EAGAIN:                            \
        case EINPROGRESS:                       \
        case EINTR:                             \
	  return status;                        \
        case MU_ERR_REPLY:                      \
        case MU_ERR_BADREPLY:                   \
	  pop3->state = MU_POP3_NO_STATE;	\
	  return status;                        \
        default:                                \
          pop3->state = MU_POP3_ERROR;		\
	  return status;			\
	}					\
    }						\
  while (0)

/* If status indicates an error, return.
  */
#define MU_POP3_CHECK_ERROR(pop3, status)	\
  do						\
    {						\
      if (status != 0)				\
	{					\
          pop3->state = MU_POP3_ERROR;		\
          return status;			\
	}					\
    }						\
  while (0)

/* Check if we got "+OK".
   In POP3 protocol and ack of "+OK" means the command was successfull.
 */
#define MU_POP3_CHECK_OK(pop3)					\
  do								\
    {								\
      if (mu_c_strncasecmp (pop3->ackbuf, "+OK", 3) != 0)	\
	{							\
          pop3->state = MU_POP3_NO_STATE;			\
          return EACCES;					\
	}							\
    }								\
  while (0)

#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_SYS_POP3_H */
