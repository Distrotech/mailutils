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

#ifndef _MIME0_H
#define _MIME0_H

#include <sys/types.h>
#include <mime.h>

#ifndef __P
#ifdef __STDC__
#define __P(args) args
#else
#define __P(args) ()
#endif
#endif /*__P */

#ifdef _cplusplus
extern "C" {
#endif

#define MIME_MAX_HDR_LEN   256
#define MIME_DFLT_BUF_SIZE	2048

/* Parser states */
#define MIME_STATE_BEGIN_LINE		1
#define MIME_STATE_SCAN_BOUNDARY	2
#define MIME_STATE_HEADERS			3

#define MIME_FLAG_MASK				0x0000ffff

/* private */
#define MIME_PARSER_ACTIVE			0x80000000
#define MIME_PARSER_HAVE_CR			0x40000000
#define MIME_NEW_MESSAGE			0x20000000

struct _mime
{
	message_t 			msg;
	header_t			hdrs;
	stream_t			stream;
	int					flags;
	char 				*content_type;

	int					tparts;
	int					nmtp_parts;		
	struct _mime_part	**mtp_parts;		/* list of parts in the msg */

	int					tmsgs;
	int 				ncap_msgs; 		
	struct _mime_part	**cap_msgs;    /* list of encapsulated msgs */
	
	/* parser state */	
	char 	*boundary;
	char	*cur_line;
	int 	line_ndx;
	int		cur_offset;
	char 	*cur_buf;
	int 	buf_size;
	char 	*header_buf;
	int		header_buf_size;
	int		header_length;
	int		body_offset;
	int		body_length;
	int		parser_state;
};

struct _mime_part
{
	char		sig[4];
	mime_t		mime;
	header_t	hdr;
	message_t 	msg;
	int 		body_offset;
	int 		body_len;
};

#ifdef _cplusplus
}
#endif

#endif /* MIME0_H */
