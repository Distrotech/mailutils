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

#include <io0.h>

struct _ts_desc {
	const char *encoding;
	int (*decode)(const char *iptr, size_t isize, char *optr, size_t *nbytes);
	int (*encode)(const char *iptr, size_t isize, char *optr, size_t *nbytes);
};

struct _trans_stream
{
	stream_t 	stream;    	 /* encoder/decoder read/writes data to here */
	int			cur_offset;
	int			offset;
	char 		*leftover;
	int			llen;
	int 		(*transcoder)(const char *iptr, size_t isize, char *optr, size_t *nbytes);
};

int _base64_decode(const char *iptr, size_t isize, char *optr, size_t *nbytes);
int _base64_encode(const char *iptr, size_t isize, char *optr, size_t *nbytes);
int _qp_decode(const char *iptr, size_t isize, char *optr, size_t *nbytes);
int _qp_encode(const char *iptr, size_t isize, char *optr, size_t *nbytes);

#define NUM_TRANSCODERS 5
struct _ts_desc tslist[NUM_TRANSCODERS] =     { { "base64", 			_base64_decode, 	_base64_encode}, 
						 						{ "quoted-printable", 	_qp_decode, 		_qp_encode},
												{ "7bit", 				NULL,				NULL},
												{ "8bit", 				NULL,				NULL},
												{ "binary", 			NULL,				NULL}
											  };

static void _trans_destroy(stream_t stream)
{
	struct _trans_stream *ts = stream->owner;	

	stream_destroy(&(ts->stream), NULL);
	if ( ts->leftover )
		free(ts->leftover);                                                                            	
   	free(ts); 
}

static int _trans_read(stream_t stream, char *optr, size_t osize, off_t offset, size_t *nbytes)
{
	struct _trans_stream *ts = stream->owner;	
	size_t isize = osize;
	char *iptr;
	int	consumed, ret;
	
	if ( nbytes == NULL || optr == NULL || osize == 0 )
		return EINVAL;

	*nbytes = 0;

	if ( offset == 0 )
		ts->cur_offset = 0;		
	if ( ( iptr = alloca(isize) ) == NULL )
		return ENOMEM;
	if ( ts->leftover ) {
		memcpy( iptr, ts->leftover, ts->llen);
		free( ts->leftover );
		ts->leftover = NULL;
		ts->offset = 0; 	// encase of error;
	}		
	if ( ( ret = stream_read(ts->stream, iptr + ts->llen, isize - ts->llen, ts->cur_offset, &osize) ) != 0 ) 
		return ret;
	ts->cur_offset += osize;
	consumed = ts->transcoder(iptr, osize + ts->llen, optr, nbytes);
	if ( ( ts->llen = ((osize + ts->llen) - consumed ) ) ) 
	{
		if ( ( ts->leftover = malloc(ts->llen) ) == NULL )
			return ENOMEM;
		memcpy(ts->leftover, iptr + consumed, ts->llen);		
	}
	return 0;
}


static int _trans_write(stream_t stream, const char *iptr, size_t isize, off_t offset, size_t *nbytes)
{
	struct _trans_stream *ts = stream->owner;	
	size_t osize = isize;
	char *optr;
	int ret;
		
	if ( nbytes == NULL || iptr == NULL || isize == 0 )
		return EINVAL;

	*nbytes = 0;

	if ( offset && ts->cur_offset != offset )
		return ESPIPE;
	if ( offset == 0 )
		ts->cur_offset = 0;		
	if ( ( optr = alloca(osize) ) == NULL )
		return ENOMEM;

	*nbytes = ts->transcoder(iptr, isize, optr, &osize);
	if ( ( ret = stream_write(ts->stream, optr, osize, ts->cur_offset, &osize) ) != 0 ) 
		return ret;

	ts->cur_offset += osize;
	
	return 0;
}

int encoder_stream_create(stream_t *stream, stream_t iostream, const char *encoding)
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

	if ( ( ret = stream_create(stream, MU_STREAM_RDWR, ts) ) != 0 )
		return ret;
	ts->transcoder = tslist[i].encode; 
	stream_set_read(*stream, _trans_read, ts );
	stream_set_write(*stream, _trans_write, ts );
	stream_set_destroy(*stream, _trans_destroy, ts );
	ts->stream = iostream;
	return 0;
}

int decoder_stream_create(stream_t *stream, stream_t iostream, const char *encoding)
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

	if ( ( ret = stream_create(stream, MU_STREAM_RDWR, ts) ) != 0 )
		return ret;
	ts->transcoder = tslist[i].decode; 
	stream_set_read(*stream, _trans_read, ts );
	stream_set_write(*stream, _trans_write, ts );
	stream_set_destroy(*stream, _trans_destroy, ts );
	ts->stream = iostream;
	return 0;
}

/*------------------------------------------------------
 * base64 encode/decode
 *----------------------------------------------------*/
static int _b64_input(char c)
{
	const char table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int 	i;

	for (i = 0; i < 64; i++) {
		if (table[i] == c)
			return i;
	}
	return -1;
}

static int _b64_output(int idx)
{
	const char table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	if (idx < 64)
		return table[idx];
	return -1;
}

int _base64_decode(const char *iptr, size_t isize, char *optr, size_t *nbytes)
{
	int i = 0, tmp = 0;
	size_t consumed = 0;
	char data[4];
		
	while ( consumed < isize ) {
		while ( ( i < 4 ) && ( consumed < isize ) ) {
			tmp = _b64_input(*iptr++);
			consumed++;
			if ( tmp != -1 )
				data[i++] = tmp;
		}
		if ( i == 4 ) { // I have a entire block of data 32 bits
			// get the output data
			*optr++ = ( data[0] << 2 ) | ( ( data[1] & 0x30 ) >> 4 );
			*optr++ = ( ( data[1] & 0xf ) << 4 ) | ( ( data[2] & 0x3c ) >> 2 );
			*optr++ = ( ( data[2] & 0x3 ) << 6 ) | data[3];
			(*nbytes) += 3;
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

int _base64_encode(const char *iptr, size_t isize, char *optr, size_t *nbytes)
{
	size_t consumed = 0;

	while (consumed < (isize - 3) && (*nbytes + 4) < isize) {
		*optr++ = _b64_output(*iptr >> 2);
		*optr++ = _b64_output(((*iptr++ & 0x3) << 4) | ((*iptr & 0xf0) >> 4));
		*optr++ = _b64_output(((*iptr++ & 0xf) << 2) | ((*iptr & 0xc0) >> 6));
		*optr++ = _b64_output(*iptr++ & 0x3f);
		consumed += 3;
		(*nbytes) += 4;
	}
	return consumed;
}

/*------------------------------------------------------
 * quoted-printable decoder/encoder
 *------------------------------------------------------*/
static const char _hexdigits[16] = "0123456789ABCDEF";

static int _ishex(int c)
{
	int i;

	if ((c == 0x0a) || (c == 0x0d))
		return 1;

	for (i = 0; i < 16; i++)
		if (c == _hexdigits[i])
			return 1;

	return 0;
}

int _qp_decode(const char *iptr, size_t isize, char *optr, size_t *nbytes)
{
	char c;
	int last_char = 0;
	size_t consumed = 0;
	
	while (consumed < isize) {
		c = *iptr++;
		if ( ((c >= 33) && (c <= 60)) || ((c >= 62) && (c <= 126)) || ((c == '=') && !_ishex(*iptr)) ) {
			*optr++ = c;
			(*nbytes)++;
			consumed++;
		}
		else if (c == '=') {
			// there must be 2 more characters before I consume this
			if ((isize - consumed) < 3) {
				return consumed;
			} 
			else {
				// you get =XX where XX are hex characters
				char 	chr[2];
				int 	new_c;

				chr[0] = *iptr++;
				if (chr[0] != 0x0a) { // LF after = means soft line break, ignore it
					chr[1] = *iptr++;
					new_c = strtoul(chr, NULL, 16);
					if (new_c == '\r')
						new_c = '\n';
					*optr++ = new_c;
					(*nbytes)++;
					consumed += 3;
				}
				else
					consumed += 2;
			}
		}
		else if (c == 0x0d) { // CR character
			// there must be at least 1 more character before I consume this
			if ((isize - consumed) < 2)
			    return (consumed);
			else {
				iptr++; // skip the LF character
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
		last_char = c;
	}
	return consumed;
}


#define QP_LINE_MAX	76
int _qp_encode(const char *iptr, size_t isize, char *optr, size_t *nbytes)
{
	int count = 0, c;
	size_t consumed = 0;
	
	while (consumed < isize && (*nbytes + 4) < isize) {
		if (count == QP_LINE_MAX) {
			*optr++ = '=';
			*optr++ = '\n';
			(*nbytes) += 2;
			count = 0;
		}

		c = *iptr++;
		consumed++;
		if ( ((c >= 32) && (c <= 60)) || ((c >= 62) && (c <= 126)) || (c == 9)) {
		    *optr++ = c;
		    (*nbytes)++;
		    count++;
		}
		else {
			if (count >= (QP_LINE_MAX - 3)) {
				// add spaces
				while (count < QP_LINE_MAX) {
					*optr++ = ' ';
					(*nbytes)++;
					count++;
				}
				consumed--;
				iptr--;
			}
			else {
				*optr++ = '=';
				*optr++ = _hexdigits[c & 0xf];
				*optr++ = _hexdigits[(c/16) & 0xf];
				(*nbytes) += 3;
				count += 3;
			}
		}
	}
	return consumed;
}
