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


#include <message0.h>
#include <mime0.h>
#include <io0.h>
#include <mime.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

/* TODO:
 *  Need to prevent re-entry into mime lib, but allow non-blocking re-entry into lib.
 *  Define mbx i/f for protocols that support mime parsing (IMAP).
 */

static int _mime_append_part( mime_t mime, int body_offset, int body_len, int encap_msg )
{
	struct _mime_part	*mime_part, **part_arr;
	int 				ret;

	if ( ( mime_part = calloc(1, sizeof(*mime_part)) ) == NULL )
		return ENOMEM;
	if ( ( ret = header_init(&mime_part->hdr, mime->header_buf, mime->header_length, mime_part) ) != 0 ) {
		free(mime_part);
		return ret;
	}
	if ( encap_msg ) {
		if ( mime->ncap_msgs >= mime->tmsgs ) {
			if ( ( part_arr = realloc(mime->cap_msgs, ( mime->tmsgs + 2 ) * sizeof(mime_part)) ) == NULL ) {
				free(mime_part);
				return ENOMEM;
			}
			mime->cap_msgs = part_arr;
			mime->tmsgs += 2;
		}
		mime->cap_msgs[mime->ncap_msgs++] = mime_part;
	}
	else {
		if ( mime->nmtp_parts >= mime->tparts ) {
			if ( ( part_arr = realloc(mime->mtp_parts, ( mime->tparts + 5 ) * sizeof(mime_part)) ) == NULL ) {
				free(mime_part);
				return ENOMEM;
			}
			mime->mtp_parts = part_arr;
			mime->tparts += 5;
		}
		mime->mtp_parts[mime->nmtp_parts++] = mime_part;
	}
	mime_part->body_len = body_len;
	mime_part->body_offset = body_offset;
	mime_part->mime = mime;
	mime->header_length = 0;
	return 0;
}

static struct _mime_part *_mime_get_owner(mime_t mime, message_t msg)
{
	int i;

	for ( i = 0; i < mime->nmtp_parts; i++ ) {
		if ( mime->mtp_parts[i] == msg->owner )
			return mime->mtp_parts[i];
	}
	return NULL;
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
    || ((c) == '\\') || ((c) == '"') || ((c) == '.') || ((c) == '[') \
    || ((c) == ']') )

static void _mime_munge_content_header(char *field_body )
{
	char *p, *e, *str = field_body;
	int quoted = 0;

	_strtrim(field_body);

	if ( ( e = p = strchr(str, ';') ) == NULL )
		return;
	e++;
	while ( *e && isspace(*e) )  /* remove space upto param */
		e++;
	memmove(p+1, e, strlen(e)+1);
	e = p+1;

	while ( *e && *e != '=' )   /* find end of value */
		e++;
	e = p = e+1;
	while ( *e && (quoted || !_ISSPECIAL(*e) || !isspace(*e) ) ) {
		if ( *e == '\\' ) {                /* escaped */
			memmove(e, e+1, strlen(e)+2);
		} else if ( *e == '\"' )
			quoted = ~quoted;
		e++;
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
		while ( *e && (quoted || !_ISSPECIAL(*e) || !isspace(*e) ) ) { 	/* skip pass value and calc len */
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
			return was_quoted ? v + 1 : v;			/* return unquted value */
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
	int 		blength, body_length, body_offset, ret;
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
						if ( mime->line_ndx >= blength ) {
							if ( ( !strncasecmp(cp2,"--", 2) && !strncasecmp(cp2+2, mime->boundary, blength) )
								|| !strncasecmp(cp2, mime->boundary, blength) ) {
								mime->parser_state = MIME_STATE_HEADERS;
								mime->flags &= ~MIME_PARSER_HAVE_CR;
								body_length = mime->cur_offset - body_offset - mime->line_ndx + 1;
								if ( mime->header_length ) /* this skips the preamble */
									_mime_append_part(mime, body_offset, body_length, FALSE );
								if ( ( cp2 + blength + 2 < cp && !strncasecmp(cp2+2+blength, "--",2) ) ||
									!strncasecmp(cp2+blength, "--",2) ) { /* very last boundary */
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
		if ( mime->flags & MIME_INCREAMENTAL_PARSER ) {
			ret = EAGAIN;
			break;
		}
	}
	mime->body_length = body_length;
	mime->body_offset = body_offset;
	if ( ret != EAGAIN ) { /* finished cleanup */
		mime->flags &= ~MIME_PARSER_ACTIVE;
		mime->body_offset = mime->body_length = mime->header_length = 0;
	}
	return ret;
}

static int _mime_message_read(stream_t stream, char *buf, size_t buflen, off_t off, size_t *nbytes)
{
	struct _mime_part *mime_part = stream->owner;
	size_t read_len;

	if ( nbytes == NULL )
		return(EINVAL);

	*nbytes = 0;
	read_len = (int)mime_part->body_len - (int)off;
	if ( read_len <= 0 )
		return 0;
	read_len = (buflen <= read_len)? buflen : read_len;

	return stream_read(mime_part->mime->stream, buf, read_len, mime_part->body_offset + off, nbytes );
}

int mime_init(mime_t *pmime, message_t msg, int flags)
{
	mime_t 	mime = NULL;
	int 	ret = 0;
	size_t 	size;

	if (pmime == NULL)
		return EINVAL;
	*pmime = NULL;
	if ( ( mime = calloc (1, sizeof (*mime)) ) == NULL )
		return ENOMEM;
	if ( msg )  {
		if ( ( ret = message_get_header(msg, &(mime->hdrs)) ) == 0 ) {
			if ( ( ret = header_get_value(mime->hdrs, "Content-Type", NULL, 0, &size) ) == 0 && size ) {
				if ( ( mime->content_type = malloc(size+1) ) == NULL )
					ret = ENOMEM;
				else if ( ( ret = header_get_value(mime->hdrs, "Content-Type", mime->content_type, size+1, 0) ) == 0 )
					_mime_munge_content_header(mime->content_type);
			} else {
				if ( ( mime->content_type = strdup("text/plain; charset=us-ascii") ) == NULL ) /* default as per spec. */
					ret = ENOMEM;
			}
			if  (ret == 0 ) {
				mime->msg = msg;
				mime->buf_size = MIME_DFLT_BUF_SIZE;
				message_get_stream(msg, &(mime->stream));
			}
		}
	}
	else { 	/* create a floating message to contain this new mime message */
		if ( ( ret = message_init( &msg, NULL ) ) == 0 ) {
			if ( ( ret = message_get_stream(msg, &(mime->stream)) ) == 0 ) {
				msg->owner = mime;
				mime->flags |= MIME_NEW_MESSAGE;
			}
		}
	}
	if ( ret != 0 ) {
		if ( mime->content_type )
			free(mime->content_type);
		free(mime);
	}
	else {
		mime->flags = (flags & MIME_FLAG_MASK);
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
				if ( mime_part->msg )
					message_destroy(&mime_part->msg, mime_part);
				else
					header_destroy(&mime_part->hdr, mime_part);
			}
		}
		if ( mime->cap_msgs != NULL ) {
			for ( i = 0; i < mime->ncap_msgs; i++ ) {
				mime_part = mime->cap_msgs[i];
				if ( mime_part->msg )
					message_destroy(&mime_part->msg, mime_part);
				else
					header_destroy(&mime_part->hdr, mime_part);
			}
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

int mime_is_multi_part(mime_t mime)
{
	if ( mime->content_type )
		return(strncasecmp("multipart", mime->content_type, strlen("multipart")) ? 0: 1);
	return 0;
}


int mime_get_part(mime_t mime, int part, message_t *msg)
{
	int 				nmtp_parts, ret = 0;
	size_t				hsize = 0;
	stream_t			stream;
	struct _mime_part	*mime_part;

	if ( ( ret = mime_get_num_parts(mime, &nmtp_parts ) ) == 0 ) {
		if ( part < 1 || part > nmtp_parts )
			return EINVAL;
		if ( nmtp_parts == 1 && mime->mtp_parts == NULL )
			*msg = mime->msg;
		else {
			mime_part = mime->mtp_parts[part-1];
			if ( ( ret = message_init(&(mime_part->msg), mime_part) ) == 0 ) {
				message_set_header(mime_part->msg, mime_part->hdr, mime_part);
				header_get_size(mime_part->hdr, &hsize);
				message_set_size (mime_part->msg, mime_part->body_len + hsize, mime_part);
				if ( ( ret = stream_init(&stream, mime_part) ) == 0 ) {
					if ( ( ret = stream_set_read(stream, _mime_message_read, mime_part) ) == 0 ) {
						if ( ( ret = message_set_stream( mime_part->msg, stream, mime_part) ) == 0 ) {
							*msg = mime_part->msg;
							return 0;
						}
					}
					stream_destroy(&stream, mime_part);
				}
				message_destroy(&mime_part->msg, mime_part);
			}
		}
	}
	return ret;
}

int mime_unencapsulate(mime_t mime, message_t msg, message_t *newmsg)
{
	size_t 				size, nbytes;
	int					ret, body_offset = 0, body_length = 0, done = 0;
	char 				*content_type, *cp;
	header_t			hdr;
	stream_t			stream;
	struct _mime_part	*mime_part;

	if ( mime == NULL || msg == NULL || newmsg == NULL || mime->flags & MIME_NEW_MESSAGE )
		return EINVAL;

	if ( mime->msg != msg && ( mime_part = _mime_get_owner( mime, msg ) ) == NULL ) /* I don't know about or own this message */
		return EPERM;

	if ( ( ret = message_get_header(msg, &hdr) ) == 0 ) {
		if ( ( ret = header_get_value(hdr, "Content-Type", NULL, 0, &size) ) == 0 && size ) {
			if ( ( content_type = malloc(size+1) ) == NULL )
				ret = ENOMEM;
			else if ( ( ret = header_get_value(hdr, "Content-Type", content_type, size+1, 0) ) == 0 ) {
				_mime_munge_content_header(content_type);
				if ( strncasecmp(content_type, "message/rfc822", strlen(content_type)) != 0 )
					ret = EINVAL;
				else {
					if ( mime_part ) {
						body_offset = mime_part->body_offset;
						body_length = mime_part->body_len;
					}
					if ( ( ret = _mime_setup_buffers(mime) ) == 0 ) {
						mime->line_ndx = 0;
						mime->cur_offset = body_offset;
						while ( !done && ( ret = stream_read(mime->stream, mime->cur_buf, mime->buf_size, mime->cur_offset, &nbytes) ) == 0 && nbytes ) {
							cp = mime->cur_buf;
							while ( nbytes ) {
								mime->cur_line[mime->line_ndx] = *cp;
								mime->line_ndx++;
								if ( *cp == '\n' ) {
									_mime_append_header_line(mime);
									if ( mime->line_ndx == 1 ) {
										done = 1;
										break;
									}
									mime->line_ndx = 0;
								}
								mime->cur_offset++;
								nbytes--;
								cp++;
							}
						}
						body_length -= mime->cur_offset - body_offset;
						body_offset = mime->cur_offset + 1;
						if ( ( ret = _mime_append_part( mime, body_offset, body_length, TRUE ) ) == 0 ) {
							mime_part = mime->cap_msgs[mime->ncap_msgs - 1];
							if ( ( ret = message_init(&(mime_part->msg), mime_part) ) == 0) {
								message_set_header(mime_part->msg, mime_part->hdr, mime_part);
								message_set_size (mime_part->msg, mime_part->body_len, mime_part);
								if ( ( ret = stream_init(&stream, mime_part) ) == 0 ) {
									if ( ( ret = stream_set_read(stream, _mime_message_read, mime_part) ) == 0 ) {
										if ( ( ret = message_set_stream( mime_part->msg, stream, mime_part) ) == 0 ) {
											*newmsg = mime_part->msg;
											return 0;
										}
									}
									stream_destroy(&stream, mime_part);
								}
								message_destroy(&mime_part->msg, mime_part);
							}
						}
					}
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
		if ( mime_is_multi_part(mime) ) {
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
	return ENOTSUP;
}

int mime_get_message(mime_t mime, message_t *msg)
{
	if ( mime == NULL || msg == NULL )
		return EINVAL;
	*msg = mime->msg;
	return 0;
}
