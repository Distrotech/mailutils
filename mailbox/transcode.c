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
#include <transcode.h>

int _base64_decode(stream_t stream, char *optr, size_t osize, off_t offset, size_t *nbytes);
int _base64_encode(stream_t stream, const char *optr, size_t osize, off_t offset, size_t *nbytes);
void _base64_destroy(transcoder_t tc);
int _qp_decode(stream_t stream, char *optr, size_t osize, off_t offset, size_t *nbytes);
int _qp_encode(stream_t stream, const char *iptr, size_t isize, off_t offset, size_t *nbytes);
int _identity_decode(stream_t stream, char *optr, size_t osize, off_t offset, size_t *nbytes);
int _identity_encode(stream_t stream, const char *iptr, size_t isize, off_t offset, size_t *nbytes);

struct _tc_desc {
	const char *encoding;
	int (*decode_read)(stream_t, char *, size_t, off_t, size_t *);
	int (*encode_write)(stream_t, const char *, size_t, off_t, size_t *);
	void (*destroy)(transcoder_t tc);
};

#define NUM_TRANSCODERS 5
struct _tc_desc tclist[NUM_TRANSCODERS] =     { { "base64", 			_base64_decode, 	_base64_encode, 	_base64_destroy },
						 						{ "quoted-printable", 	_qp_decode, 		_qp_encode, 		NULL },
												{ "7bit", 				_identity_decode, 	_identity_encode, 	NULL },
												{ "8bit", 				_identity_decode, 	_identity_encode, 	NULL },
												{ "binary", 			_identity_decode, 	_identity_encode, 	NULL }
											  };

int transcode_create(transcoder_t *ptc, char *encoding)
{
	transcoder_t tc;
	int i;

	if ( ( tc = calloc(sizeof(struct _transcoder), 1) ) == NULL )
		return ENOMEM;
	for( i = 0; i < NUM_TRANSCODERS; i++ ) {
		if ( strcasecmp( encoding, tclist[i].encoding ) == 0 )
			break;
	}
	if ( i == NUM_TRANSCODERS )
		return ENOTSUP;

	stream_create(&tc->ustream, 0, tc );
	stream_set_read(tc->ustream, tclist[i].decode_read, tc );
	stream_set_write(tc->ustream, tclist[i].encode_write, tc );
	tc->destroy = tclist[i].destroy;
	*ptc = tc;
	return 0;
}

void transcode_destroy(transcoder_t *ptc)
{
	transcoder_t tc = *ptc;

	if ( tc->destroy)
		tc->destroy(tc);
	stream_destroy(&tc->ustream, tc);
	free(tc);
	*ptc = NULL;
}

int transcode_get_stream(transcoder_t tc, stream_t *pstream)
{
	if (tc == NULL || pstream == NULL)
		return EINVAL;
	*pstream = tc->ustream;
	return 0;
}

int transcode_set_stream(transcoder_t tc, stream_t stream)
{
	if (tc == NULL || stream == NULL)
		return EINVAL;
	tc->stream = stream;
	return 0;
}

/*------------------------------------------------------
 * base64 encode/decode
 *----------------------------------------------------*/

struct _b64_decode
{
	int cur_offset;
	int offset;
	int	data_len;
	char data[4];
};

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

#if 0
static int _b64_output(int index)
{
	const char table[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	if (index < 64)
		return table[index];
	return -1;
}
#endif

void _base64_destroy(transcoder_t tc)
{
	free(tc->tcdata);
	tc->tcdata = NULL;
}

int _base64_decode(stream_t ustream, char *optr, size_t osize, off_t offset, size_t *nbytes)
{
	int i, tmp = 0, ret;
	size_t isize, consumed = 0;
	char *iptr;
	transcoder_t tc = (transcoder_t)ustream->owner;
	struct _b64_decode *decode;

	if ( nbytes == NULL || optr == NULL || osize == 0 )
		return EINVAL;

	*nbytes = 0;
	if ( ( decode = tc->tcdata ) == NULL ) {
		if ( ( decode = calloc(1, sizeof(struct _b64_decode)) ) == NULL )
			return ENOMEM;
		tc->tcdata = decode;
	}
	if ( decode->offset != offset )
		return ESPIPE;

	if ( ( iptr = alloca(osize) ) == NULL )
		return ENOMEM;
	isize = osize;

	if ( ( ret = stream_read(tc->stream, iptr, isize, decode->cur_offset, &isize) ) != 0 )
		return ret;

	decode->cur_offset += isize;
	i = decode->data_len;
	while ( consumed < isize ) {
		while ( ( i < 4 ) && ( consumed < isize ) ) {
			tmp = _b64_input(*iptr++);
			consumed++;
			if ( tmp != -1 )
				decode->data[i++] = tmp;
		}
		if ( i == 4 ) { // I have a entire block of data 32 bits
			// get the output data
			*optr++ = ( decode->data[0] << 2 ) | ( ( decode->data[1] & 0x30 ) >> 4 );
			*optr++ = ( ( decode->data[1] & 0xf ) << 4 ) | ( ( decode->data[2] & 0x3c ) >> 2 );
			*optr++ = ( ( decode->data[2] & 0x3 ) << 6 ) | decode->data[3];
			(*nbytes) += 3;
			decode->data_len = 0;
		}
		else { // I did not get all the data
			decode->data_len = i;
			decode->offset += *nbytes;
			return 0;
		}
		i = 0;
	}
	decode->offset += *nbytes;
	return 0;
}

int _base64_encode(stream_t ustream, const char *iptr, size_t osize, off_t offset, size_t *nbytes)
{
	(void)ustream; (void)iptr; (void)osize; (void)offset; (void)nbytes;
	return ENOTSUP;
}

/*------------------------------------------------------
 * quoted-printable decoder/encoder
 *------------------------------------------------------*/
struct _qp_decode
{
	int cur_offset;
	int offset;
	int	data_len;
	char data[3];
};

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


int _qp_decode(stream_t ustream, char *optr, size_t osize, off_t offset, size_t *nbytes)
{
	transcoder_t tc = ustream->owner;
	char c;
	int last_char = 0, ret;
	size_t isize, consumed = 0;
	struct _qp_decode *decode;
	char *iptr;

	if ( nbytes == NULL || optr == NULL )
		return EINVAL;

	*nbytes = 0;
	if ( ( decode = tc->tcdata ) == NULL ) {
		if ( ( decode = calloc(1, sizeof(struct _qp_decode)) ) == NULL )
			return ENOMEM;
		tc->tcdata = decode;
	}
	if ( decode->offset != offset )
		return ESPIPE;

	if ( ( iptr = alloca(osize) ) == NULL )
		return ENOMEM;
	isize = osize;

	if ( decode->data_len )
		memcpy( iptr, decode->data, decode->data_len);

	if ( ( ret = stream_read(tc->stream, iptr + decode->data_len, isize - decode->data_len, decode->cur_offset, &isize) ) != 0 )
		return ret;

	decode->data_len = 0;
	decode->cur_offset += isize;
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
				memcpy( decode->data, iptr-1, decode->data_len = isize - consumed + 1);
				return 0;
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
	decode->offset += *nbytes;
	return 0;
}


#define QP_LINE_MAX	76
struct _qp_encode
{
	char *obuf;
	int	 osize;
};

int _qp_encode(stream_t ustream, const char *iptr, size_t isize, off_t offset, size_t *nbytes)
{
#if 0
	transcoder_t tc = ustream->owner;

	static int count = 0;
	int consumed = 0, c, osize;
	char *obuf;
	struct _qp_encode *encode;


	if ( nbytes == NULL || iptr == NULL )
		return EINVAL;

	if ( isize == 0 )
		return 0;

	*nbytes = 0;
	if ( ( encode = tc->tcdata ) == NULL ) {

	}
	if ( ( encode->obuf = malloc(isize) ) == NULL )
		return ENOMEM;

	optr = obuf;
	while (consumed < isize) {
		if (count == QP_LINE_MAX) {
			*optr++ = '=';
			*optr++ = '\n';
			osize += 2;
			count = 0;
		}

		c = *iptr++;
		if ( ((c >= 32) && (c <= 60)) || ((c >= 62) && (c <= 126)) || (c == 9)) {
		    *optr++ = c;
		    osize++;
		    count++;
		}
		else {
			char chr[3];

			if (count >= (QP_LINE_MAX - 3)) {
				// add spaces
				while (count < QP_LINE_MAX) {
					*optr++ = ' ';
					osize++;
					count++;
				}
				consumed--;
				iptr--;
			}
			else {
				*optr++ = '=';
				sprintf(chr, "%02X", c);
				*optr++ = chr[0];
				*optr++ = chr[1];
				osize += 3;
				count += 3;
			}
		}
		consumed++;
		if ( osize + 4 >= isize ) {
			if (stream_write( tc->stream, obuf, osize
		}
	}
#endif
	(void)ustream; (void)iptr; (void)isize; (void)offset; (void)nbytes;
	return EINVAL;
}

/*------------------------------------------------------
 * identity decoder/encoder for 7bit, 8bit & binary
 *------------------------------------------------------*/
int _identity_decode(stream_t ustream, char *optr, size_t osize, off_t offset, size_t *nbytes)
{
	transcoder_t tc = ustream->owner;
	if ( tc->stream )
		return stream_read(tc->stream, optr, osize, offset, nbytes );
	return EINVAL;
}

int _identity_encode(stream_t ustream, const char *iptr, size_t isize, off_t offset, size_t *nbytes)
{
	transcoder_t tc = ustream->owner;
	if ( tc->stream )
		return stream_write(tc->stream, iptr, isize, offset, nbytes );
	return EINVAL;
}
