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

/* Notes:
	
 */
 

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mailutils/stream.h>

#define MU_TRANS_DECODE		1
#define MU_TRANS_ENCODE		2
#define MU_TRANS_BSIZE		2048

struct _trans_stream
{
	stream_t 	stream;    	 /* encoder/decoder read/writes data to/from here */
	int			t_offset;
	
	int 		min_size;
    int			s_offset;
	char 	   *s_buf;					/* used when read it not big enough to handle min_size for decoder/encoder */

	int			offset;		 			/* current stream offset */
	int			line_len;
		
	int			w_rhd;    				/* working buffer read head  */
	int			w_whd;					/* working buffer write head */					
	char 	    w_buf[MU_TRANS_BSIZE]; 	/* working source/dest buffer */

	int 		(*transcoder)(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);
};

struct _ts_desc 
{
	const char *encoding;

	int (*_init)(struct _trans_stream *ts, int type);
	int (*_decode)(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);
	int (*_encode)(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);
};

static int _base64_init(struct _trans_stream *ts, int type);
static int _base64_decode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);
static int _base64_encode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);

static int _qp_init(struct _trans_stream *ts, int type);
static int _qp_decode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);
static int _qp_encode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len);

#define NUM_TRANSCODERS 5
struct _ts_desc tslist[NUM_TRANSCODERS] =     { { "base64", 			_base64_init, _base64_decode, _base64_encode},
						 						{ "quoted-printable", 	_qp_init,	  _qp_decode, 	  _qp_encode},
												{ "7bit", 				NULL},
												{ "8bit", 				NULL},
												{ "binary", 			NULL}
											  };

static void 
_trans_destroy(stream_t stream)
{
	struct _trans_stream *ts = stream_get_owner(stream);

	stream_destroy(&(ts->stream), NULL);
   	free(ts);
}

static int 
_trans_read(stream_t stream, char *optr, size_t osize, off_t offset, size_t *nbytes)
{
	struct _trans_stream *ts = stream_get_owner(stream);
	size_t obytes, wbytes;
	int ret = 0, i;
	
	if ( nbytes == NULL || optr == NULL || osize == 0 )
		return EINVAL;

	*nbytes = 0;
	if ( ts->transcoder == NULL )
		return stream_read(ts->stream, optr, osize, offset, nbytes);

	if ( offset && ts->t_offset != offset )
		return ESPIPE;

	if ( offset == 0 )
		ts->s_offset = ts->t_offset = ts->w_whd = ts->w_rhd = ts->offset = ts->line_len = 0;
	
	while ( *nbytes < osize ) {
		if ( ( ts->w_rhd + ts->min_size ) >= ts->w_whd ) {
			memmove(ts->w_buf, ts->w_buf + ts->w_rhd, ts->w_whd - ts->w_rhd);
			ts->w_whd = ts->w_whd - ts->w_rhd;
			ts->w_rhd = 0;
			if ( ( ret = stream_read( ts->stream, ts->w_buf + ts->w_whd, MU_TRANS_BSIZE - ts->w_whd, ts->offset, &wbytes ) ) != 0 )
				break;
			ts->offset += wbytes;
			ts->w_whd += wbytes;
		}
		if ( (osize - *nbytes) >= ts->min_size && ts->s_offset == 0 && ts->w_whd - ts->w_rhd ) {
			ts->w_rhd += ts->transcoder(ts->w_buf + ts->w_rhd, ts->w_whd - ts->w_rhd, optr + *nbytes, osize - *nbytes, &obytes, &ts->line_len);
			if ( ts->w_rhd > ts->w_whd ) /* over shot due to padding */
				ts->w_rhd = ts->w_whd;
			*nbytes += obytes;
			ts->t_offset += obytes;
		} else {
			if ( ts->s_offset == 0 && ts->w_whd - ts->w_rhd) {
				ts->w_rhd += ts->transcoder(ts->w_buf + ts->w_rhd, ts->w_whd - ts->w_rhd, ts->s_buf, ts->min_size, &obytes, &ts->line_len);
				if ( ts->w_rhd > ts->w_whd ) /* over shot due to padding */
					ts->w_rhd = ts->w_whd;
				ts->s_offset = obytes;
			}
			for ( i = ts->s_offset; i > 0; i-- ) {
				optr[(*nbytes)++] = ts->s_buf[ts->s_offset - i];
				ts->t_offset++;
				if ( *nbytes >= osize ) {
					i--;
					memmove(ts->s_buf, &ts->s_buf[ts->s_offset - i], i );
					break;
				}
			}
			ts->s_offset = i;
		}
		if ( wbytes == 0 && ts->s_offset == 0 )
			break;
	}
	return ret;
}


static int 
_trans_write(stream_t stream, const char *iptr, size_t isize, off_t offset, size_t *nbytes)
{
	struct _trans_stream *ts = stream_get_owner(stream);
	int ret = 0;

	if ( nbytes == NULL || iptr == NULL || isize == 0 )
		return EINVAL;

	*nbytes = 0;
	if ( ts->transcoder == NULL )
		return stream_write(ts->stream, iptr, isize, offset, nbytes);

	if ( offset && ts->offset != offset )
		return ESPIPE;

	if ( offset == 0 )
		ts->t_offset = ts->w_whd = ts->w_rhd = ts->offset = 0;

	return EINVAL;
}

static int 
_trans_open (stream_t stream, const char *filename, int port, int flags)
{
	struct _trans_stream *ts = stream_get_owner(stream);

	return stream_open(ts->stream, filename, port, flags);
}

static int 
_trans_truncate (stream_t stream, off_t len)
{
	struct _trans_stream *ts = stream_get_owner(stream);

	return stream_truncate(ts->stream, len);
}

static int 
_trans_size (stream_t stream, off_t *psize)
{
	struct _trans_stream *ts = stream_get_owner(stream);
	
	return stream_size(ts->stream, psize);
}

static int 
_trans_flush (stream_t stream)
{
	struct _trans_stream *ts = stream_get_owner(stream);

	return stream_flush(ts->stream);
}
static int 
_trans_get_fd (stream_t stream, int *pfd)
{
	struct _trans_stream *ts = stream_get_owner(stream);

	return stream_get_fd(ts->stream, pfd);
}

static int 
_trans_close (stream_t stream)
{
	struct _trans_stream *ts = stream_get_owner(stream);

	return stream_close(ts->stream);
}

/*------------------------------------------------------
 * base64 encode/decode
 *----------------------------------------------------*/

static int 
_b64_input(char c)
{
	const char table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int 	i;

	for (i = 0; i < 64; i++) {
		if (table[i] == c)
			return i;
	}
	return -1;
}

static int 
_base64_init(struct _trans_stream *ts, int type)
{
	ts->min_size = 4;
	if ( ( ts->s_buf = malloc(4) ) == NULL )
		return ENOMEM;
	return 0;
}

static int 
_base64_decode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len)
{
	int i = 0, tmp = 0, pad = 0;
	size_t consumed = 0;
	char data[4];

	*nbytes = 0;
	while ( consumed < isize && (*nbytes)+3 < osize ) {
		while ( ( i < 4 ) && ( consumed < isize ) ) {
			tmp = _b64_input(*iptr++);
			consumed++;
			if ( tmp != -1 )
				data[i++] = tmp;
			else if ( *(iptr-1) == '=' ) {
				data[i++] = '\0';
				pad++;
			}
		}
		if ( i == 4 ) { // I have a entire block of data 32 bits
			// get the output data
			*optr++ = ( data[0] << 2 ) | ( ( data[1] & 0x30 ) >> 4 );
			*optr++ = ( ( data[1] & 0xf ) << 4 ) | ( ( data[2] & 0x3c ) >> 2 );
			*optr++ = ( ( data[2] & 0x3 ) << 6 ) | data[3];
			(*nbytes) += 3 - pad;
		}
		else // I did not get all the data
		{
			consumed -= i;
			return consumed;
		}
		i = 0;
	}
	return consumed;
}

#define BASE64_LINE_MAX 	77
static int 
_base64_encode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len)
{
	size_t consumed = 0;
	int pad = 0;
	const char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	
	*nbytes = 0;
	if ( isize <= 3 )
		pad = 1;
	while ( ( ( consumed + 3 ) <= isize && ( *nbytes + 4 ) <= osize ) || pad ) {
		if (*line_len == 76 ) {
			*optr++ = '\n';
			(*nbytes)++;
			(*line_len) = 0;
			if ( (*nbytes + 4 ) > osize )
				return consumed;
		}
		*optr++ = b64[iptr[0] >> 2];
		*optr++ = b64[((iptr[0] << 4) + (--isize ? (iptr[1] >> 4): 0)) & 0x3f];
		*optr++ = isize ? b64[((iptr[1] << 2) + (--isize ? (iptr[2] >> 6) : 0 )) & 0x3f] : '=';
		*optr++ = isize ? b64[iptr[2] & 0x3f] : '=';
		iptr += 3;
		consumed += 3;
		(*nbytes) += 4;
		(*line_len) +=4;
		pad = 0;
	}
	return consumed;
}

/*------------------------------------------------------
 * quoted-printable decoder/encoder
 *------------------------------------------------------*/
static const char _hexdigits[16] = "0123456789ABCDEF";

static int 
_qp_init(struct _trans_stream *ts, int type)
{
	ts->min_size = 4;
	if ( ( ts->s_buf = malloc(4) ) == NULL )
		return ENOMEM;
	return 0;
}

static int 
_ishex(int c)
{
	int i;

	if (c == '\n')
		return 0;

	for (i = 0; i < 16; i++)
		if (c == _hexdigits[i])
			return 1;

	return 0;
}

static int 
_qp_decode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len)
{
	char c;
	int last_char = 0;
	size_t consumed = 0;

	*nbytes = 0;
	while (consumed < isize && *nbytes < osize ) {
		c = *iptr++;
		if (c == '=') {
			// there must be 2 more characters before I consume this
			if ((consumed + 2) >= isize)
				return consumed;
			else {
				// you get =XX where XX are hex characters
				char 	chr[2];
				int 	new_c;

				chr[0] = *iptr++;
				if (chr[0] != '\n') { // ignore LF
					chr[1] = *iptr++;
					new_c = strtoul(chr, NULL, 16);
					*optr++ = new_c;
					(*nbytes)++;
					consumed += 3;
				}
				else
					consumed += 2;
			}
		}
		else if (c == '\r') { // CR character
			// there must be at least 1 more character before I consume this
			if ((consumed + 1) >= isize )
			    return (consumed);
			else {
				iptr++; // skip the CR character
				*optr++ = '\n';
				(*nbytes)++;
				consumed += 2;
			}
		}
		else if ((c == 9) || (c == 32)) {
			if ((last_char == 9) || (last_char == 32))
				consumed++;
			else {
				*optr++ = c;
				(*nbytes)++;
				consumed++;
			}
		}
		else {
			*optr++ = c;
			(*nbytes)++;
			consumed++;
		}
		last_char = c;
	}
	return consumed;
}


static int 
_qp_encode(const char *iptr, size_t isize, char *optr, size_t osize, size_t *nbytes, int *line_len)
{
#define QP_LINE_MAX	76
	int c;
	size_t consumed = 0;

	*nbytes = 0;
	while (consumed < isize && (*nbytes + 4) < isize) {
		if (*line_len == QP_LINE_MAX) {
			*optr++ = '=';
			*optr++ = '\n';
			(*nbytes) += 2;
			*line_len = 0;
		}

		c = *iptr++;
		consumed++;
		if ( ((c >= 32) && (c <= 60)) || ((c >= 62) && (c <= 126)) || (c == 9)) {
		    *optr++ = c;
		    (*nbytes)++;
		    (*line_len)++;
		}
		else {
			if (*line_len >= (QP_LINE_MAX - 3)) {
				// add spaces
				while (*line_len < QP_LINE_MAX) {
					*optr++ = ' ';
					(*nbytes)++;
					(*line_len)++;
				}
				consumed--;
				iptr--;
			}
			else {
				*optr++ = '=';
				*optr++ = _hexdigits[c & 0xf];
				*optr++ = _hexdigits[(c/16) & 0xf];
				(*nbytes) += 3;
				(*line_len) += 3;
			}
		}
	}
	return consumed;
}

int 
encoder_stream_create(stream_t *stream, stream_t iostream, const char *encoding)
{
	struct _trans_stream *ts;
	int i, ret;

	if ( stream == NULL || iostream == NULL || encoding == NULL )
		return EINVAL;

	if ( ( ts = calloc(sizeof(struct _trans_stream), 1) ) == NULL )
		return ENOMEM;
	for( i = 0; i < NUM_TRANSCODERS; i++ ) {
		if ( strcasecmp( encoding, tslist[i].encoding ) == 0 )
			break;
	}
	if ( i == NUM_TRANSCODERS )
		return ENOTSUP;

	if ( ( ret = stream_create(stream, MU_STREAM_RDWR |MU_STREAM_NO_CHECK, ts) ) != 0 )
		return ret;
	ts->transcoder = tslist[i]._encode;
 	/* pass thur */
	stream_set_open(*stream, _trans_open, ts );
	stream_set_close(*stream, _trans_close, ts );
	stream_set_fd(*stream, _trans_get_fd, ts );
	stream_set_truncate(*stream, _trans_truncate, ts );
	stream_set_size(*stream, _trans_size, ts );
	stream_set_flush(*stream, _trans_flush, ts );
	ts->stream = iostream;
	if ( tslist[i]._init != NULL && (ret = tslist[i]._init(ts, MU_TRANS_ENCODE)) != 0 )
		stream_destroy(stream, NULL);		
	else {
		stream_set_read(*stream, _trans_read, ts );
		stream_set_write(*stream, _trans_write, ts );
		stream_set_destroy(*stream, _trans_destroy, ts );
	}
	return ret;
}

int 
decoder_stream_create(stream_t *stream, stream_t iostream, const char *encoding)
{
	struct _trans_stream *ts;
	int i, ret;

	if ( stream == NULL || iostream == NULL || encoding == NULL )
		return EINVAL;

	if ( ( ts = calloc(sizeof(struct _trans_stream), 1) ) == NULL )
		return ENOMEM;
	for( i = 0; i < NUM_TRANSCODERS; i++ ) {
		if ( strcasecmp( encoding, tslist[i].encoding ) == 0 )
			break;
	}
	if ( i == NUM_TRANSCODERS )
		return ENOTSUP;

	if ( ( ret = stream_create(stream, MU_STREAM_RDWR |MU_STREAM_NO_CHECK, ts) ) != 0 )
		return ret;

	ts->transcoder = tslist[i]._decode;
	/* pass thur */
	stream_set_open(*stream, _trans_open, ts );
	stream_set_close(*stream, _trans_close, ts );
	stream_set_fd(*stream, _trans_get_fd, ts );
	stream_set_truncate(*stream, _trans_truncate, ts );
	stream_set_size(*stream, _trans_size, ts );
	stream_set_flush(*stream, _trans_flush, ts );
	ts->stream = iostream;
	if ( tslist[i]._init != NULL && (ret = tslist[i]._init(ts, MU_TRANS_DECODE)) != 0 )
		stream_destroy(stream, NULL);		
	else {
		stream_set_read(*stream, _trans_read, ts );
		stream_set_write(*stream, _trans_write, ts );
		stream_set_destroy(*stream, _trans_destroy, ts );
	}
	return ret;
}
