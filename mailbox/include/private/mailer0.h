/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _MAILER0_H
#define _MAILER0_H

#include <sys/types.h>
#include <mailer.h>

#ifdef _cplusplus
extern "C" {
#endif

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

// mailer states
#define MAILER_STATE_HDR		1
#define MAILER_STATE_MSG		2
#define MAILER_STATE_COMPLETE	3

// mailer messages
#define MAILER_HELO	1
#define MAILER_MAIL	2
#define MAILER_RCPT	3
#define MAILER_DATA	4
#define MAILER_RSET	5
#define MAILER_QUIT	6

#define MAILER_LINE_BUF_SIZE	1000

struct _mailer
{
	int			socket;
	char 		*hostname;
	char 		line_buf[MAILER_LINE_BUF_SIZE];
	int			offset;
	int			state;
	int			add_dot;
	stream_t	stream;
	char		last_char;
};

#ifdef _cplusplus
}
#endif

#endif /* MAILER0_H */
