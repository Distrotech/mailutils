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

#ifdef DMALLOC
#  include <dmalloc.h>
#endif

#include <sys/types.h>
#include <mailutils/mime.h>

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
#define MIME_ADDED_CONTENT_TYPE		0x10000000
#define MIME_ADDED_MULTIPART		0x08000000
#define MIME_INSERT_BOUNDARY		0x04000000

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
	char 				*boundary;
	int					cur_offset;
	int					cur_part;
	int					part_offset;
	
	/* parser state */	
	char	*cur_line;
	int 	line_ndx;
	char 	*cur_buf;
	int 	buf_size;
	char 	*header_buf;
	int		header_buf_size;
	int		header_length;
	int		body_offset;
	int		body_length;
	int		body_lines;
	int		parser_state;
};

struct _mime_part
{
	mime_t		mime;
	message_t 	msg;
	int 		offset;
	size_t		len;
	size_t		lines;
};

#ifdef _cplusplus
}
#endif

#endif /* MIME0_H */
