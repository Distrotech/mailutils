/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2010 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef _MAILUTILS_SYS_SMTP_H
# define _MAILUTILS_SYS_SMTP_H

# include <sys/types.h>
# include <sys/socket.h>
# include <mailutils/types.h>
# include <mailutils/smtp.h>

# define _MU_SMTP_ESMTP  0x01 /* Connection supports ESMTP */
# define _MU_SMTP_TRACE  0x02 /* Session trace required/enabled */
# define _MU_SMTP_ERR    0x04 /* Object in error state */
# define _MU_SMTP_MLREPL 0x08 /* Last reply was multi-line */
# define _MU_SMTP_TLS    0x10 /* TLS initiated */ 
# define _MU_SMTP_AUTH   0x20 /* Authorization passed */

#define MU_SMTP_XSCRIPT_MASK(n) (0x100<<(n))

enum mu_smtp_state
  {
    MU_SMTP_INIT,
    MU_SMTP_EHLO,
    MU_SMTP_MAIL,
    MU_SMTP_RCPT,
    MU_SMTP_MORE,
    MU_SMTP_DOT,
    MU_SMTP_QUIT,
    MU_SMTP_CLOS
  };

struct _mu_smtp
{
  int flags;
  char *domain;
  mu_stream_t carrier;
  enum mu_smtp_state state;
  mu_list_t capa;

  char replcode[4];
  char *replptr;
  
  char *rdbuf;
  size_t rdsize;

  char *flbuf;
  size_t flsize;
  
  mu_list_t mlrepl;
};

#define MU_SMTP_FSET(p,f) ((p)->flags |= (f))
#define MU_SMTP_FISSET(p,f) ((p)->flags & (f))  
#define MU_SMTP_FCLR(p,f) ((p)->flags &= ~(f))

#define MU_SMTP_CHECK_ERROR(smtp, status)	\
  do						\
    {						\
      if (status != 0)				\
	{					\
          smtp->flags |= _MU_SMTP_ERR;		\
          return status;			\
	}					\
    }						\
  while (0)

int _mu_smtp_trace_enable (mu_smtp_t smtp);
int _mu_smtp_trace_disable (mu_smtp_t smtp);
int _mu_smtp_xscript_level (mu_smtp_t smtp, int xlev);

#endif
