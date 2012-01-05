/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

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

#ifndef _MAILUTILS_MSGSET_H
#define _MAILUTILS_MSGSET_H

# include <mailutils/types.h>
  
#ifdef __cplusplus
extern "C" {
#endif

struct mu_msgrange
{
  size_t msg_beg;
  size_t msg_end;
};

/* Message numbers start with 1.  MU_MSGNO_LAST denotes the last
   message. */
#define MU_MSGNO_LAST   0

#define MU_MSGSET_NUM   0      /* Message set operates on sequence numbers */   
#define MU_MSGSET_UID   1      /* Message set operates on UIDs */

#define MU_MSGSET_MODE_MASK 0x0f
  
int mu_msgset_create (mu_msgset_t *pmsgset, mu_mailbox_t mbox, int mode);
int mu_msgset_get_list (mu_msgset_t msgset, mu_list_t *plist);
int mu_msgset_get_iterator (mu_msgset_t msgset, mu_iterator_t *pitr);

int mu_msgset_add_range (mu_msgset_t set, size_t beg, size_t end, int mode);
int mu_msgset_sub_range (mu_msgset_t set, size_t beg, size_t end, int mode);
int mu_msgset_add (mu_msgset_t a, mu_msgset_t b);
int mu_msgset_sub (mu_msgset_t a, mu_msgset_t b);
int mu_msgset_aggregate (mu_msgset_t set);
int mu_msgset_clear (mu_msgset_t set);
void mu_msgset_free (mu_msgset_t set);
void mu_msgset_destroy (mu_msgset_t *set);
  
int mu_msgset_parse_imap (mu_msgset_t set, int mode, const char *s, char **end);

int mu_msgset_print (mu_stream_t str, mu_msgset_t msgset);
  
int mu_msgset_locate (mu_msgset_t msgset, size_t n,
		      struct mu_msgrange const **prange);

int mu_msgset_negate (mu_msgset_t msgset, mu_msgset_t *pnset);

int mu_msgset_count (mu_msgset_t mset, size_t *pcount);
int mu_msgset_is_empty (mu_msgset_t mset);
  
typedef int (*mu_msgset_msgno_action_t) (size_t _n, void *_call_data);
typedef int (*mu_msgset_message_action_t) (size_t _n, mu_message_t _msg,
					   void *_call_data);

#define MU_MSGSET_FOREACH_FORWARD  0x00
#define MU_MSGSET_FOREACH_BACKWARD 0x10

int mu_msgset_foreach_num (mu_msgset_t _msgset, int _flags,
			   mu_msgset_msgno_action_t _action,
			   void *_call_data);
  
int mu_msgset_foreach_dir_msgno (mu_msgset_t _msgset, int _dir,
				 mu_msgset_msgno_action_t _action,
				 void *_data);
int mu_msgset_foreach_msgno (mu_msgset_t _msgset,
			     mu_msgset_msgno_action_t _action,
			     void *_call_data);
int mu_msgset_foreach_dir_msguid (mu_msgset_t _msgset, int _dir,
				  mu_msgset_msgno_action_t _action,
				  void *_data);
int mu_msgset_foreach_msguid (mu_msgset_t _msgset,
			      mu_msgset_msgno_action_t _action,
			      void *_data);
  
int mu_msgset_foreach_dir_message (mu_msgset_t _msgset, int _dir,
				   mu_msgset_message_action_t _action,
				   void *_call_data);
int mu_msgset_foreach_message (mu_msgset_t _msgset,
			       mu_msgset_message_action_t _action,
			       void *_call_data);

  
#ifdef __cplusplus
}
#endif

#endif
