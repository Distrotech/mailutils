/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <message.h>
#include <io.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HDR_LEN 256
#define BUF_SIZE	2048

struct _msg_info {
	char	*buf;
	size_t	nbytes;
	char	*header_buf;
	int		header_len;
	int		header_size;
	header_t hdr;	
	message_t msg;
	int		ioffset;
	int 	ooffset;
	char	line[MAX_HDR_LEN];
	int		line_ndx;
};

#define MSG_HDR "Content-Type: %s\nContent-Transfer-Encoding: %s\n\n"

int message_create_attachment(const char *content_type, const char *encoding, const char *filename, message_t *newmsg)
{
	header_t  hdr;
	body_t    body;
	stream_t  fstream = NULL, tstream = NULL;
	char *header;
	int ret;

	if ( ( ret = message_create(newmsg, NULL) ) == 0 ) {
		if ( content_type == NULL ) 
			content_type = "text/plain";
		if ( encoding == NULL )
			encoding = "7bit";
		if ( ( header = alloca(strlen(MSG_HDR) + strlen(content_type) + strlen(encoding)) ) == NULL )
			ret = ENOMEM;
		else {
			sprintf(header, MSG_HDR, content_type, encoding);
			if ( ( ret = header_create( &hdr, header, strlen(header), *newmsg ) ) == 0 ) {
				message_get_body(*newmsg, &body);
				if ( ( ret = file_stream_create(&fstream, filename, MU_STREAM_READ) ) == 0 ) {
					if ( ( ret = encoder_stream_create(&tstream, fstream, encoding) ) == 0 ) {
						body_set_stream(body, tstream, *newmsg);
						message_set_header(*newmsg, hdr, NULL);
					}
				}
			}
		}
	}
	if ( ret ) {
		if ( *newmsg ) 
			message_destroy(newmsg, NULL);
		if ( hdr )
			header_destroy(&hdr, NULL);
		if ( fstream ) 
			stream_destroy(&fstream, NULL);
	}
	return ret;
}


static int _attachment_setup(struct _msg_info **info, message_t msg, stream_t *stream, void **data)
{
	int 				sfl, ret;
	body_t				body;

	if ( ( ret = message_get_body(msg, &body) ) != 0 || 
		 ( ret = body_get_stream(body, stream) ) != 0 ) 
		return ret;
	stream_get_flags(*stream, &sfl);
	if ( data == NULL && (sfl & MU_STREAM_NONBLOCK) )
		return EINVAL;
	if ( data )
		*info = *data;		
	if ( *info == NULL ) {
		if ( ( *info = calloc(1, sizeof(struct _msg_info)) ) == NULL )
			return ENOMEM;
	}
	if ( ( (*info)->buf = malloc(BUF_SIZE) ) == NULL ) {
		free(*info);
		return ENOMEM;
	}
	return 0;
}

static void _attachment_free(struct _msg_info *info, int free_message) {
	if ( info->buf ) 
		free(info->buf);
	if ( info->header_buf )
		free(info->header_buf);
	if ( free_message ) {
		if ( info->msg )
			message_destroy(&(info->msg), NULL);
		else if ( info->hdr ) 
			header_destroy(&(info->hdr), NULL);
	}
	free(info);
}

int message_save_attachment(message_t msg, const char *filename, void **data)
{
	stream_t			stream;
	header_t			hdr;
	struct _msg_info	*info = NULL;
	int 				ret;
	size_t				size;
	char 				*content_encoding;
	
	if ( msg == NULL || filename == NULL)
		return EINVAL;

	if ( ( data == NULL || *data == NULL) && ( ret = message_get_header(msg, &hdr) ) == 0 ) {
		header_get_value(hdr, "Content-Transfer-Encoding", NULL, 0, &size);
		if ( size ) {
			if ( ( content_encoding = alloca(size+1) ) == NULL )
				ret = ENOMEM;
			header_get_value(hdr, "Content-Transfer-Encoding", content_encoding, size+1, 0);
		}
	}
	if ( ret == 0 && ( ret = _attachment_setup( &info, msg, &stream, data) ) != 0 )
		return ret;
	
	if ( ret != EAGAIN && info )
		_attachment_free(info, ret);
	return ret;	
}

#if 0
int message_encapsulate(message_t msg, message_t *newmsg, void **data)
{
	stream_t			stream;
	char 				*header;
	struct _msg_info	*info = NULL;
	int ret;

	if ( msg == NULL || newmsg == NULL)
		return EINVAL;
		
	if ( ( ret = message_create(&(info->msg), NULL) ) == 0 ) {
		header = "Content-Type: message/rfc822\nContent-Transfer-Encoding: 7bit\n\n";
		if ( ( ret = header_create( &(info->hdr), header, strlen(header), msg ) ) == 0 ) {
			message_set_header(info->msg, info->hdr, NULL);
		}
	}
	return ret;	
}
#endif

/* If the message interface parsed headers on write this would be easy */

int message_unencapsulate(message_t msg, message_t *newmsg, void **data)
{
	size_t 				size, nbytes;
	int					ret = 0, header_done = 0;
	char 				*content_type, *cp;
	header_t			hdr;
	body_t				body;
	stream_t			istream, ostream;
	struct _msg_info	*info = NULL;

	if ( msg == NULL || newmsg == NULL)
		return EINVAL;
		
	if ( (data == NULL || *data == NULL ) && ( ret = message_get_header(msg, &hdr) ) == 0 ) {
		header_get_value(hdr, "Content-Type", NULL, 0, &size);
		if ( size ) {
			if ( ( content_type = alloca(size+1) ) == NULL )
				ret = ENOMEM;
			header_get_value(hdr, "Content-Type", content_type, size+1, 0);
			if ( strncasecmp(content_type, "message/rfc822", strlen(content_type)) != 0 )
				ret = EINVAL;
		} else
			return EINVAL;
	}
	if ( ret == 0 && ( ret = _attachment_setup( &info, msg, &istream, data) ) != 0 )
		return ret;

	if ( ret == 0  && info->hdr == NULL ) {
		while ( !header_done && ( ret = stream_read(istream, info->buf, BUF_SIZE, info->ioffset, &info->nbytes) ) == 0 && info->nbytes ) {
			cp = info->buf;
			while ( info->nbytes && !header_done ) {
				info->line[info->line_ndx] = *cp;
				info->line_ndx++;
				if ( *cp == '\n' ) {
					if ( info->header_len + info->line_ndx > info->header_size) {
						char *nhb;
						if ( ( nhb = realloc( info->header_buf, info->header_len + info->line_ndx + 128 ) ) == NULL ) {
							header_done = 1;
							ret = ENOMEM;
							break;
						}
						info->header_buf = nhb;
						info->header_size = info->header_len + info->line_ndx + 128;
					}
					info->header_len += info->line_ndx;
					memcpy(info->header_buf, info->line, info->line_ndx);
					if ( info->line_ndx == 1 ) {
						header_done = 1;						
						break;
					}
					info->line_ndx = 0;
				}
				info->ioffset++;
				info->nbytes--;
				cp++;
			}
		}
	}
	if ( ret == 0 && info->msg == NULL ) {
		if ( ( ret = message_create(&(info->msg), NULL) ) == 0)
			if ( ( ret = header_create(&(info->hdr), info->header_buf, info->header_len, info->msg) ) == 0 ) 
				ret = message_set_header(info->msg, hdr, NULL);
	}
	if ( ret == 0 ) {
		message_get_body(info->msg, &body);
		body_get_stream( body, &ostream);
		if ( info->nbytes )
			memmove( info->buf, info->buf + (BUF_SIZE - info->nbytes), info->nbytes);
		while ( info->nbytes || ( ( ret = stream_read(istream, info->buf, BUF_SIZE, info->ioffset, &info->nbytes) ) == 0 && info->nbytes ) ) {
			info->ioffset += info->nbytes;
			while( info->nbytes ) {
				if ( ( ret = stream_write(ostream, info->buf, info->nbytes, info->ooffset, &nbytes ) ) != 0 )
					break;
				info->nbytes -= nbytes;
				info->ooffset += nbytes;
			}
		}
	}
	if ( ret != EAGAIN && info )
		_attachment_free(info, ret);
	return ret;	
}

