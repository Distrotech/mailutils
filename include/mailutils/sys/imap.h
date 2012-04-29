/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_SYS_IMAP_H
# define _MAILUTILS_SYS_IMAP_H

# include <mailutils/sys/folder.h>
# include <mailutils/sys/mailbox.h>
# include <mailutils/sys/registrar.h>
# include <mailutils/sys/auth.h>
# include <mailutils/imapio.h>
# include <mailutils/imap.h>

# ifdef __cplusplus
extern "C" {
# endif

#define MU_IMAP_RESP  0x01
#define MU_IMAP_TRACE 0x02
#define MU_IMAP_XSCRIPT_MASK(n) (1<<((n)+1))
#define MU_IMAP_SET_XSCRIPT_MASK(imap,n)	\
  do						\
    (imap)->flags |= MU_IMAP_XSCRIPT_MASK (n);	\
  while (0)
#define MU_IMAP_CLR_XSCRIPT_MASK(imap,n)	\
  do						\
    (imap)->flags &= ~MU_IMAP_XSCRIPT_MASK (n);	\
  while (0)
#define MU_IMAP_IS_XSCRIPT_MASK(imap,n)         \
  ((imap)->flags & MU_IMAP_XSCRIPT_MASK (n))
  
enum mu_imap_client_state
  {
    MU_IMAP_CLIENT_READY,
    MU_IMAP_CLIENT_ERROR,
    MU_IMAP_CLIENT_CONNECT_RX,
    MU_IMAP_CLIENT_GREETINGS,
    MU_IMAP_CLIENT_CAPABILITY_RX,
    MU_IMAP_CLIENT_LOGIN_RX,
    MU_IMAP_CLIENT_LOGOUT_RX,
    MU_IMAP_CLIENT_ID_RX,
    MU_IMAP_CLIENT_SELECT_RX,
    MU_IMAP_CLIENT_STATUS_RX,
    MU_IMAP_CLIENT_NOOP_RX,
    MU_IMAP_CLIENT_FETCH_RX,
    MU_IMAP_CLIENT_STORE_RX,
    MU_IMAP_CLIENT_DELETE_RX,
    MU_IMAP_CLIENT_RENAME_RX,
    MU_IMAP_CLIENT_CLOSE_RX,
    MU_IMAP_CLIENT_UNSELECT_RX,
    MU_IMAP_CLIENT_CHECK_RX,
    MU_IMAP_CLIENT_COPY_RX,
    MU_IMAP_CLIENT_EXPUNGE_RX,
    MU_IMAP_CLIENT_APPEND_RX,
    MU_IMAP_CLIENT_LIST_RX,
    MU_IMAP_CLIENT_SUBSCRIBE_RX,
    MU_IMAP_CLIENT_UNSUBSCRIBE_RX,
    MU_IMAP_CLIENT_LSUB_RX,
    MU_IMAP_CLIENT_STARTTLS_RX,
    MU_IMAP_CLIENT_CLOSING
  };

enum mu_imap_response
  {
    MU_IMAP_OK,
    MU_IMAP_NO,
    MU_IMAP_BAD
  };
  
struct _mu_imap
  {
    int flags;

    /* Holds the recent response */
    enum mu_imap_response response;
    /* The recent response code */
    int response_code;
    
    /* Error string (if any) */
    char *errstr;
    size_t errsize;
    
    enum mu_imap_client_state client_state;
    enum mu_imap_session_state session_state;

    /* Tag */
    size_t tag_len;  /* Length of the command tag */
    int *tag_buf;    /* Tag number (BCD) */
    char *tag_str;   /* String representation (tag_len + 1 bytes, asciiz) */
    
    mu_list_t capa;
    mu_imapio_t io;

    char *mbox_name;  /* Name of the currently opened mailbox */
    int mbox_writable:1; /* Is it open read/write? */
    struct mu_imap_stat mbox_stat;  /* Stats obtained from it */

    /* Callbacks */
    struct
    {
      mu_imap_callback_t action;
      void *data;
    } callback[_MU_IMAP_CB_MAX];
};

enum imap_eltype
  {
    imap_eltype_string,
    imap_eltype_list
  };

struct imap_list_element
{
  enum imap_eltype type;
  union
  {
    mu_list_t list;
    char *string;
  } v;
};

#define MU_IMAP_FSET(p,f) ((p)->flags |= (f))
#define MU_IMAP_FISSET(p,f) ((p)->flags & (f))  
#define MU_IMAP_FCLR(p,f) ((p)->flags &= ~(f))

int _mu_imap_init (mu_imap_t imap);
int _mu_imap_trace_enable (mu_imap_t imap);
int _mu_imap_trace_disable (mu_imap_t imap);
int _mu_imap_xscript_level (mu_imap_t imap, int xlev);

typedef void (*mu_imap_response_action_t) (mu_imap_t imap, mu_list_t resp,
					   void *data);

struct imap_command
{
  int session_state;
  char *capa;
  int rx_state;
  int argc;
  char const **argv;
  char const *extra;
  mu_msgset_t msgset;
  void (*tagged_handler) (mu_imap_t);
  mu_imap_response_action_t untagged_handler;
  void *untagged_handler_data;
};

int mu_imap_gencom (mu_imap_t imap, struct imap_command *cmd);
  
/* If status indicates an error, return.
  */
#define MU_IMAP_CHECK_ERROR(imap, status)			\
  do								\
    {								\
      if (status != 0)						\
	{							\
          imap->client_state = MU_IMAP_CLIENT_ERROR;		\
          return status;					\
	}							\
    }								\
  while (0)

/* Check if status indicates an error.
   If the error is recoverable just return the status.
   Otherwise, set the error state and return the status
 */
#define MU_IMAP_CHECK_EAGAIN(imap, status)		\
  do							\
    {							\
      switch (status)					\
	{						\
        case 0:						\
	  break;					\
        case EAGAIN:					\
        case EINPROGRESS:				\
        case EINTR:					\
	  return status;				\
        case MU_ERR_REPLY:				\
        case MU_ERR_BADREPLY:				\
	  imap->client_state = MU_IMAP_CLIENT_READY;	\
	  return status;				\
        default:					\
          imap->client_state = MU_IMAP_CLIENT_ERROR;	\
	  return status;				\
	}						\
    }							\
  while (0)

int _mu_imap_seterrstr (mu_imap_t imap, const char *str, size_t len);
int _mu_imap_seterrstrz (mu_imap_t imap, const char *str);
void _mu_imap_clrerrstr (mu_imap_t imap);

int _mu_imap_tag_next (mu_imap_t imap);
int _mu_imap_tag_clr (mu_imap_t imap);


int _mu_imap_untagged_response_to_list (mu_imap_t imap, mu_list_t *plist);
int _mu_imap_process_untagged_response (mu_imap_t imap, mu_list_t list,
					mu_imap_response_action_t fun,
					void *data);
int _mu_imap_process_tagged_response (mu_imap_t imap, mu_list_t resp);
 
int _mu_imap_response (mu_imap_t imap, mu_imap_response_action_t fun,
		       void *data);
  
int _mu_imap_list_element_is_string (struct imap_list_element *elt,
				     const char *str);
int _mu_imap_list_element_is_nil (struct imap_list_element *elt);

int _mu_imap_list_nth_element_is_string (mu_list_t list, size_t n,
					 const char *str);
  
int _mu_imap_collect_flags (struct imap_list_element *arg, int *res);

struct imap_list_element *_mu_imap_list_at (mu_list_t list, int idx);
  
int _mu_imap_parse_fetch_response (mu_list_t resp, mu_list_t *result_list);

void _mu_close_handler (mu_imap_t imap);

/* ----------------------------- */
/* URL Auxiliaries               */
/* ----------------------------- */
int _mu_imap_url_init (mu_url_t url);
int _mu_imaps_url_init (mu_url_t url);

/* ----------------------------- */
/* Mailbox interface             */
/* ----------------------------- */

int _mu_imap_mailbox_init (mu_mailbox_t mailbox);

#define _MU_IMAP_MSG_SCANNED   0x01 /* Message has already been scanned */
#define _MU_IMAP_MSG_CACHED    0x02 /* Message is cached */
#define _MU_IMAP_MSG_LINES     0x04 /* Number of lines is computed */
#define _MU_IMAP_MSG_ATTRCHG   0x08 /* Message attributes has changed */

struct _mu_imap_message
{
  int flags;
  size_t uid;               /* Message UID */   
  int attr_flags;           /* Attributes */
  mu_off_t offset;          /* Offset in the message cache stream */
  mu_off_t body_start;      /* Start of message, relative to offset */
  mu_off_t body_end;        /* End of message, relative to offset */  
  size_t header_lines;      /* Number of lines in the header */
  size_t body_lines;        /* Number of lines in the body */
  size_t message_size;      /* Message size */
  size_t message_lines;     /* Number of lines in the message */
  struct mu_imapenvelope *env; /* IMAP envelope */
  mu_message_t message;     /* Pointer to the message structure */ 
  struct _mu_imap_mailbox *imbx; /* Back pointer.  */
};

#define _MU_IMAP_MBX_UPTODATE  0x01

struct _mu_imap_mailbox
{
  int flags;
  mu_off_t total_size;        /* Total mailbox size. */
  struct mu_imap_stat stats;     /* Mailbox statistics */ 
  struct _mu_imap_message *msgs; /* Array of messages */
  size_t msgs_cnt;               /* Number of used slots in msgs */
  size_t msgs_max;               /* Number of slots in msgs */
  mu_stream_t cache;          /* Message cache stream */
  int last_error;             /* Last error code */
  mu_mailbox_t mbox;
};

# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_IMAP_H */
