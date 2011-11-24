/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010, 2011 Free Software Foundation, Inc.

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

enum mu_imap_client_state
  {
    MU_IMAP_NO_STATE,
    MU_IMAP_ERROR,
    MU_IMAP_CONNECT,
    MU_IMAP_GREETINGS,
    MU_IMAP_CONNECTED,
    MU_IMAP_CAPABILITY_RX,
    MU_IMAP_LOGIN_RX,
    MU_IMAP_LOGOUT_RX,
    MU_IMAP_ID_RX,
    MU_IMAP_SELECT_RX,
    MU_IMAP_STATUS_RX,
    MU_IMAP_CLOSING
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

    /* Holds the recect response code */
    enum mu_imap_response resp_code;
    
    /* Error string (if any) */
    char *errstr;
    size_t errsize;
    
    enum mu_imap_state state;
    enum mu_imap_state imap_state;

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

/* If status indicates an error, return.
  */
#define MU_IMAP_CHECK_ERROR(imap, status)	\
  do						\
    {						\
      if (status != 0)				\
	{					\
          imap->state = MU_IMAP_ERROR;		\
          return status;			\
	}					\
    }						\
  while (0)

/* Check if status indicates an error.
   If the error is recoverable just return the status.
   Otherwise, set the error state and return the status
 */
#define MU_IMAP_CHECK_EAGAIN(imap, status)	\
  do						\
    {						\
      switch (status)				\
	{					\
        case 0:                                 \
	  break;				\
        case EAGAIN:                            \
        case EINPROGRESS:                       \
        case EINTR:                             \
        case MU_ERR_REPLY:                      \
        case MU_ERR_BADREPLY:                   \
	  return status;                        \
        default:                                \
          imap->state = MU_IMAP_ERROR;		\
	  return status;			\
	}					\
    }						\
  while (0)

int _mu_imap_seterrstr (mu_imap_t imap, const char *str, size_t len);
int _mu_imap_seterrstrz (mu_imap_t imap, const char *str);
void _mu_imap_clrerrstr (mu_imap_t imap);

int _mu_imap_tag_next (mu_imap_t imap);
int _mu_imap_tag_clr (mu_imap_t imap);


typedef void (*mu_imap_response_action_t) (mu_imap_t imap, mu_list_t resp,
					   void *data);

int _mu_imap_untagged_response_to_list (mu_imap_t imap, mu_list_t *plist);
int _mu_imap_process_untagged_response (mu_imap_t imap, mu_list_t list,
					mu_imap_response_action_t fun,
					void *data);
 
int _mu_imap_response (mu_imap_t imap, mu_imap_response_action_t fun,
		       void *data);
  
int _mu_imap_list_element_is_string (struct imap_list_element *elt,
				     const char *str);
int _mu_imap_collect_flags (struct imap_list_element *arg, int *res);

struct imap_list_element *_mu_imap_list_at (mu_list_t list, int idx);
  
  
# ifdef __cplusplus
}
# endif

#endif /* _MAILUTILS_SYS_IMAP_H */
