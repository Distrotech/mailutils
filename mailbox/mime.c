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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mailutils/message.h>
#include <mailutils/stream.h>
#include <mailutils/body.h>
#include <mailutils/header.h>
#include <mime0.h>

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

/* TODO:
 *  Need to prevent re-entry into mime lib, but allow non-blocking re-entry into lib.
 */

static int _mime_is_multipart_digest(mime_t mime)
{
	if ( mime->content_type )
		return(strncasecmp("multipart/digest", mime->content_type, strlen("multipart/digest")) ? 0: 1);
	return 0;
}

static int _mime_append_part(mime_t mime, message_t msg, int offset, int len, int lines)
{
	struct _mime_part	*mime_part, **part_arr;
	int 				ret;
	size_t				size;
	header_t			hdr;

	if ( ( mime_part = calloc(1, sizeof(*mime_part)) ) == NULL )
		return ENOMEM;

	if ( mime->nmtp_parts >= mime->tparts ) {
		if ( ( part_arr = realloc(mime->mtp_parts, ( mime->tparts + 5 ) * sizeof(mime_part)) ) == NULL ) {
			free(mime_part);
			return ENOMEM;
		}
		mime->mtp_parts = part_arr;
		mime->tparts += 5;
	}
	mime->mtp_parts[mime->nmtp_parts++] = mime_part;
	if ( msg == NULL ) {
		if ( ( ret = message_create(&(mime_part->msg), mime_part) ) == 0 ) {
			if ( ( ret = header_create(&hdr, mime->header_buf, mime->header_length, mime_part->msg) ) != 0 ) {
				message_destroy(&mime_part->msg, mime_part);
				free(mime_part);
				return ret;
			}
			message_set_header(mime_part->msg, hdr, mime_part);
		} else {
			free(mime_part);
			return ret;
		}
		mime->header_length = 0;
		if ( ( ret = header_get_value(hdr, "Content-Type", NULL, 0, &size) ) != 0 || size == 0 ) {
			if ( _mime_is_multipart_digest(mime) )
				header_set_value(hdr, "Content-Type", "message/rfc822", 0);
			else
				header_set_value(hdr, "Content-Type", "text/plain", 0);
		}
		mime_part->len = len;
		mime_part->lines = lines;
		mime_part->offset = offset;
	} else {
		message_size(msg, &mime_part->len);
		message_lines(msg, &mime_part->lines);
		if ( mime->nmtp_parts > 1 )
			mime_part->offset = mime->mtp_parts[mime->nmtp_parts-2]->len;
		mime_part->msg = msg;
	}
	mime_part->mime = mime;
	return 0;
}

static char *_strltrim(char *str)
{
	char    *p;

	for (p = str; isspace(*p) && *p != '\0'; ++p)
		;
	return((p != str) ? memmove(str, p, strlen(p)+1) : str);
}

static char *_strttrim(char *str)
{
	char    *p;

	for (p = str + strlen(str) - 1; isspace(*p) && p >= str; --p)
		;
	*++p = '\0';
	return(str);
}

char *_strtrim(char *str);
#define _strtrim(str) _strltrim(_strttrim(str))

#define _ISSPECIAL(c) ( \
    ((c) == '(') || ((c) == ')') || ((c) == '<') || ((c) == '>') \
    || ((c) == '@') || ((c) == ',') || ((c) == ';') || ((c) == ':') \
    || ((c) == '\\') || ((c) == '.') || ((c) == '[') \
    || ((c) == ']') )

static void _mime_munge_content_header(char *field_body )
{
	char *p, *e, *str = field_body;
	int quoted = 0;

	_strtrim(field_body);

	if ( ( e = strchr(str, ';') ) == NULL )
		return;
	while( *e == ';' ) {
		p = e;
		e++;
		while ( *e && isspace(*e) )  /* remove space upto param */
			e++;
		memmove(p+1, e, strlen(e)+1);
		e = p+1;

		while ( *e && *e != '=' )   /* find end of value */
			e++;
		e = p = e+1;
		while ( *e && (quoted || ( !_ISSPECIAL(*e) && !isspace(*e) ) ) ) {
			if ( *e == '\\' ) {                /* escaped */
				memmove(e, e+1, strlen(e)+2);
			} else if ( *e == '\"' )
				quoted = ~quoted;
			e++;
		}
	}
}

static char *_mime_get_param(char *field_body, const char *param, int *len)
{
	char *str, *p, *v, *e;
	int quoted = 0, was_quoted = 0;

	if ( len == NULL || ( str = field_body ) == NULL )
		return NULL;

	p = strchr(str, ';' );
	while ( p ) {
		p++;
		if ( ( v = strchr(p, '=' ) ) == NULL )
			break;
		*len = 0;
		v = e = v + 1;
		while ( *e && (quoted || ( !_ISSPECIAL(*e) && !isspace(*e) ) ) ) { 	/* skip pass value and calc len */
			if ( *e == '\"' )
				quoted = ~quoted, was_quoted = 1;
			else
				(*len)++;
			e++;
		}
		if ( strncasecmp(p, param, strlen(param)) ) {	/* no match jump to next */
			p = strchr(e, ';' );
			continue;
		}
		else
			return was_quoted ? v + 1 : v;			/* return unquoted value */
	}
	return NULL;
}

static int _mime_setup_buffers(mime_t mime)
{
	if ( mime->cur_buf == NULL && ( mime->cur_buf = malloc( mime->buf_size ) ) == NULL ) {
		return ENOMEM;
	}
	if ( mime->cur_line == NULL && ( mime->cur_line = malloc( MIME_MAX_HDR_LEN ) ) == NULL ) {
		free(mime->cur_buf);
		return ENOMEM;
	}
	return 0;
}

static void _mime_append_header_line(mime_t mime)
{
	if ( mime->header_length + mime->line_ndx > mime->header_buf_size) {
		char *nhb;
		if ( ( nhb = realloc( mime->header_buf, mime->header_length + mime->line_ndx + 128 ) ) == NULL )
			return;
		mime->header_buf = nhb;
		mime->header_buf_size = mime->header_length + mime->line_ndx + 128;
	}
	memcpy(mime->header_buf + mime->header_length, mime->cur_line, mime->line_ndx);
	mime->header_length += mime->line_ndx;
}

static int _mime_parse_mpart_message(mime_t mime)
{
	char 		*cp, *cp2;
	int 		blength, body_length, body_offset, body_lines, ret;
	size_t		nbytes;

	if ( !(mime->flags & MIME_PARSER_ACTIVE) ) {
		char *boundary;
		int len;

		if ( ( ret = _mime_setup_buffers(mime) ) != 0 )
			return ret;
		if ( ( boundary = _mime_get_param(mime->content_type, "boundary", &len) ) == NULL )
			return EINVAL;
		if ( ( mime->boundary = calloc(1, len + 1) ) == NULL )
			return ENOMEM;
		strncpy(mime->boundary, boundary, len );

		mime->cur_offset = 0;
		mime->line_ndx = 0;
		mime->parser_state = MIME_STATE_SCAN_BOUNDARY;
		mime->flags |= MIME_PARSER_ACTIVE;
	}
	body_length = mime->body_length;
	body_offset = mime->body_offset;
	body_lines = mime->body_lines;

	while ( ( ret = stream_read(mime->stream, mime->cur_buf, mime->buf_size, mime->cur_offset, &nbytes) ) == 0 && nbytes ) {
		cp = mime->cur_buf;
		while ( nbytes ) {
			mime->cur_line[mime->line_ndx] = *cp;
			if ( *cp == '\n' ) {
				switch( mime->parser_state ) {
					case MIME_STATE_BEGIN_LINE:
						mime->cur_line[0] = *cp;
						mime->line_ndx = 0;
						mime->parser_state = MIME_STATE_SCAN_BOUNDARY;
						break;
					case MIME_STATE_SCAN_BOUNDARY:
						cp2 = mime->cur_line[0] == '\n' ? mime->cur_line + 1 : mime->cur_line;
						blength = strlen(mime->boundary);
						if ( mime->header_length )
							body_lines++;
						if ( mime->line_ndx >= blength ) {
							if ( ( !strncasecmp(cp2,"--", 2) && !strncasecmp(cp2+2, mime->boundary, blength) )
								|| !strncasecmp(cp2, mime->boundary, blength) ) {
								mime->parser_state = MIME_STATE_HEADERS;
								mime->flags &= ~MIME_PARSER_HAVE_CR;
								body_length = mime->cur_offset - body_offset - mime->line_ndx + 1;
								if ( mime->header_length ) /* this skips the preamble */
									_mime_append_part(mime, NULL, body_offset, body_length, body_lines);
								if ( ( cp2 + blength + 2 < cp && !strncasecmp(cp2+2+blength, "--",2) ) ||
									!strncasecmp(cp2+blength, "--",2) ) { /* very last boundary */
									mime->parser_state = MIME_STATE_BEGIN_LINE;
									mime->header_length = 0;
									break;
								}
								mime->line_ndx = -1; /* headers parsing requires empty line */
								break;
							}
						}
						mime->line_ndx = 0;
						mime->cur_line[0] = *cp; /* stay in this state but leave '\n' at begining */
						break;
					case MIME_STATE_HEADERS:
						mime->line_ndx++;
						_mime_append_header_line(mime);
						if ( mime->line_ndx == 1 || mime->cur_line[0] == '\r' ) {
							mime->parser_state = MIME_STATE_BEGIN_LINE;
							body_offset = mime->cur_offset + 1;
							body_lines = 0;
						}
						mime->line_ndx = -1;
						break;
				}
			}
			mime->line_ndx++;
			if ( mime->line_ndx >= MIME_MAX_HDR_LEN ) {
				mime->line_ndx = 0;
				mime->parser_state = MIME_STATE_BEGIN_LINE;
			}
			mime->cur_offset++;
			nbytes--;
			cp++;
		}
	}
	mime->body_lines = body_lines;
	mime->body_length = body_length;
	mime->body_offset = body_offset;
	if ( ret != EAGAIN ) { /* finished cleanup */
		if ( mime->header_length ) /* this skips the preamble */
			_mime_append_part(mime, NULL, body_offset, body_length, body_lines);
		mime->flags &= ~MIME_PARSER_ACTIVE;
		mime->body_offset = mime->body_length = mime->header_length = mime->body_lines = 0;
	}
	return ret;
}

/*------ Mime message functions for READING a multipart message -----*/

static int _mimepart_body_read(stream_t stream, char *buf, size_t buflen, off_t off, size_t *nbytes)
{
	body_t				body = stream_get_owner(stream);
	message_t			msg = body_get_owner(body);
	struct _mime_part 	*mime_part = message_get_owner(msg);
	size_t 				read_len;

	if ( nbytes == NULL )
		return(EINVAL);

	*nbytes = 0;
	read_len = (int)mime_part->len - (int)off;
	if ( read_len <= 0 )
		return 0;
	read_len = (buflen <= read_len)? buflen : read_len;

	return stream_read(mime_part->mime->stream, buf, read_len, mime_part->offset + off, nbytes );
}

static int _mimepart_body_fd(stream_t stream, int *fd)
{
	body_t				body = stream_get_owner(stream);
	message_t			msg = body_get_owner(body);
	struct _mime_part 	*mime_part = message_get_owner(msg);

	return stream_get_fd(mime_part->mime->stream, fd);
}

static int _mimepart_body_size (body_t body, size_t *psize)
{
	message_t 			msg = body_get_owner(body);
	struct _mime_part	*mime_part = message_get_owner(msg);

	if (mime_part == NULL)
		return EINVAL;
	if (psize)
		*psize = mime_part->len;
	return 0;
}

static int _mimepart_body_lines (body_t body, size_t *plines)
{
	message_t			msg = body_get_owner(body);
	struct _mime_part 	*mime_part = message_get_owner(msg);

	if (mime_part == NULL)
		return EINVAL;
	if (plines)
		*plines = mime_part->lines;
	return 0;
}

/*------ Mime message/header functions for CREATING multipart message -----*/
static int _mime_set_content_type(mime_t mime)
{
	char content_type[256];
	char boundary[128];

	if ( mime->nmtp_parts > 1 ) {
		if ( mime->flags & MIME_ADDED_MULTIPART )
			return 0;
		if ( mime->flags & MIME_MULTIPART_MIXED )
			strcpy(content_type, "multipart/mixed; boundary=");
		else
			strcpy(content_type, "multipart/alternative; boundary=");
		if ( mime->boundary == NULL ) {
			sprintf (boundary,"%ld-%ld=:%ld",random (),time (0), getpid ());
			if ( ( mime->boundary = strdup(boundary) ) == NULL )
				return ENOMEM;
		}
		strcat(content_type, "\"");
		strcat(content_type, mime->boundary);
		strcat(content_type, "\"");
		mime->flags |= MIME_ADDED_MULTIPART;
	} else {
		if ( (mime->flags & (MIME_ADDED_CONTENT_TYPE|MIME_ADDED_MULTIPART)) == MIME_ADDED_CONTENT_TYPE )
			return 0;
		mime->flags &= ~MIME_ADDED_MULTIPART;
		strcpy(content_type, "text/plain; charset=us-ascii");
	}
	mime->flags |= MIME_ADDED_CONTENT_TYPE;
	return header_set_value(mime->hdrs, "Content-Type", content_type, 1);
}

static int _mime_body_read(stream_t stream, char *buf, size_t buflen, off_t off, size_t *nbytes)
{
	body_t		body = stream_get_owner(stream);
	message_t	msg = body_get_owner(body);
	mime_t		mime = message_get_owner(msg);
	int 		ret = 0, len;
	size_t		part_nbytes = 0;
	stream_t	msg_stream = NULL;

	if ( mime->nmtp_parts == 0 )
		return EINVAL;

	if ( off == 0 ) { 			/* reset message */
		mime->cur_offset = 0;
		mime->cur_part = 0;
	}

	if ( off != mime->cur_offset )
		return ESPIPE;

	if ( nbytes )
		*nbytes = 0;

	if ( mime->cur_part == mime->nmtp_parts )
		return 0;

	if ( ( ret = _mime_set_content_type(mime) ) == 0 ) {
		do {
			len = 0;
			if ( mime->nmtp_parts > 1 && ( mime->flags & MIME_INSERT_BOUNDARY || mime->cur_offset == 0 ) ) {
				mime->cur_part++;
				len = 2;
				buf[0] = buf[1] = '-';
				buf+=2;
				len += strlen(mime->boundary);
				strcpy(buf, mime->boundary);
				buf+= strlen(mime->boundary);
				if ( mime->cur_part == mime->nmtp_parts ) {
					len+=2;
					buf[0] = buf[1] = '-';
					buf+=2;
				}
				len++;
				buf[0] = '\n';
				buf++;
				mime->flags &= ~MIME_INSERT_BOUNDARY;
				buflen =- len;
				mime->part_offset = 0;
				if ( mime->cur_part == mime->nmtp_parts ) {
					if ( nbytes )
						*nbytes += len;
					mime->cur_offset +=len;
					break;
				}
			}
			message_get_stream(mime->mtp_parts[mime->cur_part]->msg, &msg_stream);
			ret = stream_read(msg_stream, buf, buflen, mime->part_offset, &part_nbytes );
    		len += part_nbytes;
			mime->part_offset += part_nbytes;
			if ( nbytes )
				*nbytes += len;
			mime->cur_offset += len;
			if ( ret == 0 && part_nbytes == 0 )
				mime->flags |= MIME_INSERT_BOUNDARY;
		} while( ret == 0 && part_nbytes == 0 );
	}
	return ret;
}

static int _mime_body_fd(stream_t stream, int *fd)
{
	body_t 		body = stream_get_owner(stream);
	message_t 	msg = body_get_owner(body);
	mime_t 		mime = message_get_owner(msg);
	stream_t	msg_stream = NULL;

	if ( mime->nmtp_parts == 0 || mime->cur_offset == 0 )
		return EINVAL;
	message_get_stream(mime->mtp_parts[mime->cur_part]->msg, &msg_stream);
	return stream_get_fd(msg_stream, fd);
}

static int _mime_body_size (body_t body, size_t *psize)
{
	message_t	msg = body_get_owner(body);
	mime_t		mime = message_get_owner(msg);
	int 		i;
	size_t		size;

	if ( mime->nmtp_parts == 0 )
		return EINVAL;

	for ( i=0;i<mime->nmtp_parts;i++ ) {
		message_size(mime->mtp_parts[i]->msg, &size);
		*psize+=size;
		if ( mime->nmtp_parts > 1 )		/* boundary line */
			*psize+= strlen(mime->boundary) + 3;
	}
	if ( mime->nmtp_parts > 1 )		/* ending boundary line */
		*psize+= 2;

	return 0;
}

static int _mime_body_lines (body_t body, size_t *plines)
{
	message_t	msg = body_get_owner(body);
	mime_t		mime = message_get_owner(msg);
	int 		i;
	size_t		lines;

	if ( mime->nmtp_parts == 0 )
		return EINVAL;

	for ( i = 0; i < mime->nmtp_parts; i++ ) {
		message_lines(mime->mtp_parts[i]->msg, &lines);
		plines+=lines;
		if ( mime->nmtp_parts > 1 )		/* boundary line */
			plines++;
	}
	return 0;
}

int mime_create(mime_t *pmime, message_t msg, int flags)
{
	mime_t 	mime = NULL;
	int 	ret = 0;
	size_t 	size;
	body_t	body;

	if (pmime == NULL)
		return EINVAL;
	*pmime = NULL;
	if ( ( mime = calloc(1, sizeof (*mime)) ) == NULL )
		return ENOMEM;
	if ( msg )  {
		if ( ( ret = message_get_header(msg, &(mime->hdrs)) ) == 0 ) {
			if ( ( ret = header_get_value(mime->hdrs, "Content-Type", NULL, 0, &size) ) == 0 && size ) {
				if ( ( mime->content_type = malloc(size+1) ) == NULL )
					ret = ENOMEM;
				else if ( ( ret = header_get_value(mime->hdrs, "Content-Type", mime->content_type, size+1, 0) ) == 0 )
					_mime_munge_content_header(mime->content_type);
			} else {
				ret = 0;
				if ( ( mime->content_type = strdup("text/plain; charset=us-ascii") ) == NULL ) /* default as per spec. */
					ret = ENOMEM;
			}
			if  (ret == 0 ) {
				mime->msg = msg;
				mime->buf_size = MIME_DFLT_BUF_SIZE;
				message_get_body(msg, &body);
				body_get_stream(body, &(mime->stream));
			}
		}
	}
	else {
		mime->flags |= MIME_NEW_MESSAGE;
	}
	if ( ret != 0 ) {
		if ( mime->content_type )
			free(mime->content_type);
		free(mime);
	}
	else {
		mime->flags |= (flags & MIME_FLAG_MASK);
		*pmime = mime;
	}
	return ret;
}

void mime_destroy(mime_t *pmime)
{
	mime_t mime;
	struct _mime_part *mime_part;
	int i;

	if (pmime && *pmime) {
		mime = *pmime;
		if ( mime->mtp_parts != NULL ) {
			for ( i = 0; i < mime->nmtp_parts; i++ ) {
				mime_part = mime->mtp_parts[i];
				if ( mime_part->msg ) {
					message_destroy(&mime_part->msg, mime_part);
					free (mime_part);
				}
			}
			free (mime->mtp_parts);
		}
		if ( mime->content_type )
			free(mime->content_type);
		if ( mime->cur_buf )
			free(mime->cur_buf);
		if ( mime->cur_line )
			free(mime->cur_line);
		if ( mime->boundary )
			free(mime->boundary);
		if ( mime->header_buf )
			free(mime->header_buf);
		free (mime);
		*pmime = NULL;
	}
}

int mime_get_part(mime_t mime, int part, message_t *msg)
{
	int 				nmtp_parts, ret = 0;
	stream_t			stream;
	body_t				body;
	struct _mime_part	*mime_part;

	if ( ( ret = mime_get_num_parts(mime, &nmtp_parts ) ) == 0 ) {
		if ( part < 1 || part > nmtp_parts )
			return EINVAL;
		if ( nmtp_parts == 1 && mime->mtp_parts == NULL )
			*msg = mime->msg;
		else {
			mime_part = mime->mtp_parts[part-1];
			if ( ( ret = body_create(&body, mime_part->msg) ) == 0 ) {
				body_set_size (body, _mimepart_body_size, mime_part->msg);
				body_set_lines (body, _mimepart_body_lines, mime_part->msg);
				if ( ( ret = stream_create(&stream, MU_STREAM_READ, body) ) == 0 ) {
					stream_set_read(stream, _mimepart_body_read, body);
					stream_set_fd(stream, _mimepart_body_fd, body);
					body_set_stream(body, stream, mime_part->msg);
					message_set_body(mime_part->msg, body, mime_part);
					*msg = mime_part->msg;
					return 0;
				}
			}
		}
	}
	return ret;
}

int mime_get_num_parts(mime_t mime, int *nmtp_parts)
{
	int 		ret = 0;

	if ( mime->nmtp_parts == 0 || mime->flags & MIME_PARSER_ACTIVE ) {
		if ( mime_is_multipart(mime) ) {
			if ( ( ret = _mime_parse_mpart_message(mime) ) != 0 )
				return(ret);
		} else
			mime->nmtp_parts = 1;
	}
	*nmtp_parts = mime->nmtp_parts;
	return(ret);

}

int mime_add_part(mime_t mime, message_t msg)
{
	if ( mime == NULL || msg == NULL || ( mime->flags & MIME_NEW_MESSAGE ) == 0 )
		return EINVAL;
	return _mime_append_part(mime, msg, 0, 0, 0);
}

int mime_get_message(mime_t mime, message_t *msg)
{
	stream_t body_stream;
	body_t	body;
	int ret = 0;

	if ( mime == NULL || msg == NULL )
		return EINVAL;
	if ( mime->msg == NULL ) {
		if ( ( mime->flags & MIME_NEW_MESSAGE ) == 0 )
			return EINVAL;
		if ( ( ret = message_create(&(mime->msg), mime) ) == 0 ) {
			if ( ( ret = header_create(&(mime->hdrs), NULL, 0, mime->msg) ) == 0 ) {
				message_set_header(mime->msg, mime->hdrs, mime);
				header_set_value(mime->hdrs, "MIME-Version", "1.0", 0);
				if ( ( ret = _mime_set_content_type(mime) ) == 0 )  {
					if ( ( ret = body_create(&body, mime->msg) ) == 0 ) {
						message_set_body(mime->msg, body, mime);
						body_set_size (body, _mime_body_size, mime->msg);
						body_set_lines (body, _mime_body_lines, mime->msg);
						if ( ( ret = stream_create(&body_stream, MU_STREAM_READ, body) ) == 0 ) {
							stream_set_read(body_stream, _mime_body_read, body);
							stream_set_fd(body_stream, _mime_body_fd, body);
							body_set_stream(body, body_stream, mime->msg);
							*msg = mime->msg;
							return 0;
						}
					}
				}
			}
			message_destroy(&(mime->msg), mime);
		}
	}
	if ( ret == 0 )
		*msg = mime->msg;
	return ret;
}

int mime_is_multipart(mime_t mime)
{
	if ( mime->content_type )
		return(strncasecmp("multipart", mime->content_type, strlen("multipart")) ? 0: 1);
	return 0;
}
