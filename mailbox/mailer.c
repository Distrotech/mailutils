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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>

#include <mailer0.h>

int 	_mailer_sock_connect(char *host, int port);
char	*_mailer_find_mailbox(char *addr);
int 	_mailer_send_command(mailer_t ml, message_t msg, int cmd);
char 	*nb_fgets(char *buf, int size, int s);
const char 	*nb_fprintf(int s, const char *format, ...);
static int _mailer_rctp(mailer_t ml, char *str);

#define nb_read		read
#define nb_write 	write
#define BUFFSIZE 	4096

int
mailer_create(mailer_t *pml, message_t msg)
{
	mailer_t ml;

	(void)msg;
	if (!pml)
	  return EINVAL;

	ml = calloc (1, sizeof (*ml));
	if (ml == NULL)
		return (ENOMEM);

	*pml = ml;

	return (0);
}

int
mailer_destroy(mailer_t *pml)
{
	mailer_t ml;

	if (!pml)
		return (EINVAL);
	ml = *pml;
	if (ml->hostname)
		free(ml->hostname);
	mailer_disconnect(ml);
	free(ml);
	*pml = NULL;

	return (0);
}

int
mailer_connect(mailer_t ml, char *host)
{
	if (!ml || !host)
		return (EINVAL);

	if ((ml->socket = _mailer_sock_connect(host, 25)) < 0)
		return (-1);
	do
	{
		nb_fgets(ml->line_buf, MAILER_LINE_BUF_SIZE, ml->socket); /* read header line */
	} while ( strlen(ml->line_buf) > 4 && *(ml->line_buf+3) == '-');

	return (0);
}

int
mailer_disconnect(mailer_t ml)
{
	if (!ml || (ml->socket != -1))
		return (EINVAL);

	close(ml->socket);
	return (0);
}

int
mailer_send_header(mailer_t ml, message_t msg)
{
	header_t 	hdr;
	char		buf[64];

	if (!ml || !msg || (ml->socket == -1))
		return (EINVAL);

	if (!ml->hostname)
	{
	    if (gethostname(buf, 64) < 0)
	    	return (-1);
	    ml->hostname = strdup(buf);
	}

	if (_mailer_send_command(ml, msg, MAILER_HELO) != 0)
		return (-1);
	if (_mailer_send_command(ml, msg, MAILER_MAIL) != 0)
		return (-1);
	if (_mailer_send_command(ml, msg, MAILER_RCPT) != 0)
		return (-1);
	if (_mailer_send_command(ml, msg, MAILER_DATA) != 0)
		return (-1);

	message_get_header(msg, &hdr);
	header_get_stream(hdr, &(ml->stream));

	ml->state = MAILER_STATE_HDR;

	return (0);
}

int
mailer_send_message(mailer_t ml, message_t msg)
{
	int 	status, data_len = 0;
	size_t consumed = 0, len = 0;
	char 	*data, *p, *q;

	if (!ml || !msg || (ml->socket == -1))
		return (EINVAL);

	// alloca
	if (!(data = alloca(MAILER_LINE_BUF_SIZE)))
		return (ENOMEM);

	memset(data, 0, 1000);
	if ((status = stream_read(ml->stream, data, MAILER_LINE_BUF_SIZE, ml->offset, &len)) != 0)
		return (-1);

	if ((len == 0) && (ml->state == MAILER_STATE_HDR))
	{
		ml->state = MAILER_STATE_MSG;
		ml->offset = 0;
		message_get_stream(msg, &(ml->stream));
		return (1);
	}
	else if (len == 0)
	{
		strcpy(ml->line_buf, "\r\n.\r\n");
		consumed = strlen(data);
	}
	else
	{
		p = data;
		q = ml->line_buf;
		memset(ml->line_buf, 0, MAILER_LINE_BUF_SIZE);
		while (consumed < len)
		{
			// RFC821: if the first character on a line is a '.' you must add an
			//         extra '.' to the line which will get stipped off at the other end
			if ((*p == '.') && (ml->last_char == '\n'))
				ml->add_dot = 1;
			ml->last_char = *p;
			*q++ = *p++; 	// store the character
			data_len++;		// increase the length by 1
			consumed++;
			if (((MAILER_LINE_BUF_SIZE - data_len) > 1) && (ml->add_dot == 1))
			{
				*q++ = '.';
				data_len++;
				ml->add_dot = 0;
			}
		}
	}

	ml->offset += consumed;
	nb_fprintf(ml->socket, "%s\r\n", ml->line_buf);

	if (len == 0)
	{
		ml->state = MAILER_STATE_COMPLETE;
		return (0);
	}

	return (consumed);
}

int
_mailer_sock_connect(char *host, int port)
{
	struct sockaddr_in saddr;
	struct hostent *hp;
	int s;

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			return (-1);
	if ((hp = gethostbyname(host)) == 0)
			return (-1);
	memcpy(&saddr.sin_addr, hp->h_addr, hp->h_length);
	if (connect(s, (struct sockaddr *)&saddr, sizeof(saddr)) == 0)
			return (s);
	close(s);

	return (-1);
}

char *
_mailer_find_mailbox(char *addr)
{
    char *p, *c;
    p = addr;
    if ( (c = strchr( p, '<')) != 0)
    {
        p = c+1;
        if ( (c = strchr( p, '>')) )
            *c = '\0';
    }
    else if ( (c = strchr( p, '(' )) != 0 )
    {
        --c;
        while ( c > p && *c && isspace( *c ) ) {
            *c = '\0';
            --c;
        }
    }
    return p;
}

static int
_mailer_rctp(mailer_t ml, char *str)
{
	char 		*p, *c = NULL, *q = NULL;

	for (q = p = str; q && *p; p = q+1)
	{
		if ( (q = strchr( p, ',')) )
			*q = '\0';
		while ( p && *p && isspace( *p ) )
			p++;
		c = strdup(p);
		p = _mailer_find_mailbox(c);
		nb_fprintf(ml->socket, "RCPT TO:<%s>\r\n", p);
		free(c);
		nb_fgets(ml->line_buf, sizeof(ml->line_buf), ml->socket);
		if (strncmp(ml->line_buf, "250", 3))
			return (-strtol(ml->line_buf, 0, 10));
	}
	return (0);
}

int
_mailer_send_command(mailer_t ml, message_t msg, int cmd)
{
	header_t	hdr;
	char 		*p;
	char		str[128];
	size_t		str_len;
	const char		*success = "250";

	switch (cmd)
	{
		case MAILER_HELO:
			nb_fprintf(ml->socket, "HELO %s\r\n", ml->hostname);
			break;
		case MAILER_MAIL:
			message_get_header(msg, &hdr);
			header_get_value(hdr, MU_HEADER_FROM, str, 128, &str_len);
			str[str_len] = '\0';
			p = _mailer_find_mailbox(str);
			nb_fprintf(ml->socket, "MAIL From: %s\r\n", p);
			break;
		case MAILER_RCPT:
			message_get_header(msg, &hdr);
			header_get_value(hdr, MU_HEADER_TO, str, 128, &str_len);
			str[str_len] = '\0';
			if (_mailer_rctp(ml, str) == -1)
				return (-1);
			header_get_value(hdr, MU_HEADER_CC, str, 128, &str_len);
			str[str_len] = '\0';
			if (_mailer_rctp(ml, str) == -1)
				return (-1);
			return (0);
			break;
		case MAILER_DATA:
			nb_fprintf(ml->socket, "DATA\r\n");
			success = "354";
			break;
		case MAILER_RSET:
			nb_fprintf(ml->socket, "RSET\r\n");
			break;
		case MAILER_QUIT:
			nb_fprintf(ml->socket, "QUIT\r\n");
			success = "221";
			break;
	}

	nb_fgets(ml->line_buf, sizeof(ml->line_buf), ml->socket);
	if (strncmp(ml->line_buf, success, 3) == 0)
		return (0);
	else
		return (-strtol(ml->line_buf, 0, 10));
}

char *
nb_fgets( char *buf, int size, int s )
{
	static char *buffer[25];
	char *p, *b, *d;
	int bytes, i;
	int flags;

	if ( !buffer[s] && !( buffer[s] = calloc( BUFFSIZE+1, 1 ) ) )
		return 0;
	bytes = i = strlen( p = b = buffer[s] );
	*( d = buf ) = '\0';
	for ( ; i-- > 0; p++ )
		{
		if ( *p == '\n' )
			{
			char c = *( p+1 );

			*( p+1 ) = '\0';
			strcat( d, b );
			*( p+1 ) = c;
			memmove( b, p+1, i+1 );
			return buf;
			}
		}
	flags = fcntl( s, F_GETFL );
	fcntl( s, F_SETFL, O_NONBLOCK );
	while ( bytes <= size )
		{
		fd_set fds;

		FD_ZERO( &fds );
		FD_SET( s, &fds );
		select( s+1, &fds, 0, 0, 0 ); /* we really don't care what it returns */
		if ( ( i = nb_read( s, p, BUFFSIZE - bytes ) ) == -1 )
			{
			*b = '\0';
			return 0;
			}
		else if ( i == 0 )
			{
			*( p+1 ) = '\0';
			strcat( d, b );
			*b = '\0';
			fcntl( s, F_SETFL, flags );
			return strlen( buf ) ? buf : 0;
			}
		*( p+i ) = '\0';
		bytes += i;
		for ( ; i-- > 0; p++ )
			{
			if ( *p == '\n' )
				{
				char c = *( p+1 );

				*( p+1 ) = '\0';
				strcat( d, b );
				*( p+1 ) = c;
				memmove( b, p+1, i+1 );
				fcntl( s, F_SETFL, flags );
				return buf;
				}
			}
		if ( bytes == BUFFSIZE )
			{
			memcpy( d, b, BUFFSIZE );
			d += BUFFSIZE;
			size -= BUFFSIZE;
			bytes = 0;
			*( p = b ) = '\0';
			}
		}
	memcpy( d, b, size );
	memmove( b, b+size, strlen( b+size )+1 );
	fcntl( s, F_SETFL, flags );
	return buf;
}

const char *
nb_fprintf( int s, const char *format, ... )
{
	char buf[MAILER_LINE_BUF_SIZE];
	va_list vl;
	int i;

	va_start( vl, format );
	vsprintf( buf, format, vl );
	va_end( vl );
	i = strlen( buf );
	if ( nb_write( s, buf, i ) != i )
		return 0;
	return format;
}
