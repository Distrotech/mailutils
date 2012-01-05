/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2011-2012 Free Software Foundation, Inc.

   GNU Mailutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GNU Mailutils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_IMAPIO_H
# define _MAILUTILS_IMAPIO_H

#ifdef __cplusplus
extern "C" {
#endif

# include <mailutils/types.h>
# include <mailutils/datetime.h>  
# include <time.h>

#define MU_IMAPIO_CLIENT 0
#define MU_IMAPIO_SERVER 1

int mu_imapio_create (mu_imapio_t *iop, mu_stream_t str, int server);
void mu_imapio_free (mu_imapio_t io);
void mu_imapio_destroy (mu_imapio_t *pio);
  
int mu_imapio_getline (mu_imapio_t io);
int mu_imapio_set_xscript_level (mu_imapio_t io, int xlev);  
int mu_imapio_get_words (mu_imapio_t io, size_t *pwc, char ***pwv);

int mu_imapio_send (mu_imapio_t io, const char *buf, size_t bytes);
int mu_imapio_printf (mu_imapio_t io, const char *fmt, ...);
int mu_imapio_send_literal_string (mu_imapio_t io, const char *buffer);
int mu_imapio_send_literal_stream (mu_imapio_t io, mu_stream_t stream,
				   mu_off_t size);  
int mu_imapio_send_qstring (mu_imapio_t io, const char *buffer);
int mu_imapio_send_qstring_unfold (mu_imapio_t io, const char *buffer,
				   int unfold);
int mu_imapio_send_msgset (mu_imapio_t io, mu_msgset_t msgset);

int mu_imapio_send_command_v (mu_imapio_t io, const char *tag,
			      mu_msgset_t msgset,
			      int argc, char const **argv, const char *extra);
int mu_imapio_send_command (mu_imapio_t io, const char *tag,
			    mu_msgset_t msgset, char const *cmd, ...);
int mu_imapio_send_command_e (mu_imapio_t io, const char *tag,
			      mu_msgset_t msgset, char const *cmd, ...);
  
int mu_imapio_send_flags (mu_imapio_t io, int flags);
int mu_imapio_send_time (mu_imapio_t io, struct tm *tm,
			 struct mu_timezone *tz);
  
int mu_imapio_trace_enable (mu_imapio_t io);
int mu_imapio_trace_disable (mu_imapio_t io);
int mu_imapio_get_trace (mu_imapio_t io);
void mu_imapio_trace_payload (mu_imapio_t io, int val);
int mu_imapio_get_trace_payload (mu_imapio_t io);

int mu_imapio_get_streams (mu_imapio_t io, mu_stream_t *streams);
int mu_imapio_set_streams (mu_imapio_t io, mu_stream_t *streams);
  
int mu_imapio_getbuf (mu_imapio_t io, char **pptr, size_t *psize);

int mu_imapio_reply_string (mu_imapio_t io, size_t start, char **pbuf);

int mu_imapio_last_error (mu_imapio_t io);
void mu_imapio_clearerr (mu_imapio_t io);
  
#ifdef __cplusplus
}
#endif

#endif /* _MAILUTILS_IMAPIO_H */
  
