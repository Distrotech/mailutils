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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <net0.h>
#include <io0.h>
#include <tcp.h>

static int _tcp_close(void *data);

static int _tcp_doconnect(struct _tcp_instance *tcp)
{
	int 					flgs, ret;
	size_t					namelen;
	struct sockaddr_in 		peer_addr;
	struct hostent 			*phe;
	struct sockaddr_in      soc_addr;
	
	switch( tcp->state ) {
		case TCP_STATE_INIT:
			if ( tcp->fd == -1 ) {
				if ( ( tcp->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
					return errno;
			}
			if ( tcp->options->non_block ) {
				flgs = fcntl(tcp->fd, F_GETFL);
				flgs |= O_NONBLOCK;
				fcntl(tcp->fd, F_SETFL, flgs);
			}
			tcp->state = TCP_STATE_RESOLVING;
		case TCP_STATE_RESOLVING:
			if ( tcp->host == NULL || tcp->port == -1 ) 
				return EINVAL;
			tcp->address = inet_addr(tcp->host);
			if (tcp->address == INADDR_NONE) {
				phe = gethostbyname(tcp->host);
				if ( !phe ) {
					_tcp_close(tcp);
					return EINVAL;
				}
				tcp->address = *(((unsigned long **)phe->h_addr_list)[0]);
			}
			tcp->state = TCP_STATE_RESOLVE;
		case TCP_STATE_RESOLVE:
			memset (&soc_addr, 0, sizeof (soc_addr));
			soc_addr.sin_family = AF_INET;
			soc_addr.sin_port = htons(tcp->port);  
			soc_addr.sin_addr.s_addr = tcp->address;
	
			if ( ( connect(tcp->fd, (struct sockaddr *) &soc_addr, sizeof(soc_addr)) ) == -1 ) {
				ret = errno;
				if ( ret == EINPROGRESS || ret == EAGAIN ) {
					tcp->state = TCP_STATE_CONNECTING;
					ret = EAGAIN;
				} else
					_tcp_close(tcp);
				return ret;
			}
			tcp->state = TCP_STATE_CONNECTING;
		case TCP_STATE_CONNECTING:
			namelen = sizeof (peer_addr);
			if ( getpeername (tcp->fd, (struct sockaddr *)&peer_addr, &namelen) == 0 )
				tcp->state = TCP_STATE_CONNECTED;
			else {
				ret = errno;
				_tcp_close(tcp);
				return ret;
			}
			break;
	}
	return 0;
}

static int _tcp_get_fd(stream_t stream, int *fd)
{
	struct _tcp_instance *tcp = stream->owner;

	if ( fd == NULL )
		return EINVAL;
		
	if ( tcp->fd == -1 ) {
		if ( ( tcp->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
			return errno;
	}
	*fd = tcp->fd;
	return 0;	
}

static int _tcp_read(stream_t stream, char *buf, size_t buf_size, off_t offset, size_t *br)
{
	struct _tcp_instance *tcp = stream->owner;
	
	offset;
	if ( br == NULL )	
		return EINVAL;
	*br = 0;
	if ( ( *br = recv(tcp->fd, buf, buf_size, 0) ) == -1 ) {
		*br = 0;
		return errno;
	}
	return 0;
}

static int _tcp_write(stream_t stream, const char *buf, size_t buf_size, off_t offset, size_t *bw)
{
	struct _tcp_instance *tcp = stream->owner;
	
	offset;
	if ( bw == NULL )	
		return EINVAL;
	*bw = 0;
	if ( ( *bw = send(tcp->fd, buf, buf_size, 0) ) == -1 ) {
		*bw = 0;
		return errno;
	}
	return 0;
}

static int _tcp_new(void *netdata, net_t parent, void **data)
{
	struct _tcp_instance *tcp;
	
	if ( parent )		/* tcp must be top level api */
		return EINVAL;	

	if ( ( tcp = malloc(sizeof(*tcp)) ) == NULL )
		return ENOMEM;
	tcp->options = (struct _tcp_options *)netdata;
	tcp->fd = -1;
	tcp->host = NULL;
	tcp->port = -1;
	tcp->state = TCP_STATE_INIT;
	stream_create(&tcp->stream, tcp);
	stream_set_read(tcp->stream, _tcp_read, tcp);
	stream_set_write(tcp->stream, _tcp_write, tcp);
	stream_set_fd(tcp->stream, _tcp_get_fd, tcp);
	*data = tcp;
	return 0;
}

static int _tcp_connect(void *data, const char *host, int port)
{
	struct _tcp_instance *tcp = data;
	
	if ( tcp->state == TCP_STATE_INIT ) {
		tcp->port = port;	
		if ( ( tcp->host = strdup(host) ) == NULL )
			return ENOMEM;
	}
	if ( tcp->state < TCP_STATE_CONNECTED )
		return _tcp_doconnect(tcp);
	return 0;
}

static int _tcp_get_stream(void *data, stream_t *stream)
{
	struct _tcp_instance *tcp = data;

	*stream = tcp->stream;
	return 0;
}

static int _tcp_close(void *data)
{
	struct _tcp_instance *tcp = data;

	if ( tcp->fd != -1 )
		close(tcp->fd);
	tcp->fd = -1;
	tcp->state = TCP_STATE_INIT;
	return 0;
}

static int _tcp_free(void **data)
{
	struct _tcp_instance *tcp;

	if ( data == NULL || *data == NULL ) 
		return EINVAL;
	tcp = *data;

	if ( tcp->host ) 
		free(tcp->host);
	if ( tcp->fd != -1 )
		close(tcp->fd);
	
	free(*data);
	*data = NULL;
	return 0;
}

static struct _net_api _tcp_net_api = {
	_tcp_new,
	_tcp_connect,
	_tcp_get_stream,
	_tcp_close,
	_tcp_free
};

int _tcp_create(void **netdata, struct _net_api **netapi) 
{
	struct _tcp_options *options;
	
	if ( ( options = malloc(sizeof(*options)) ) == NULL )
		return ENOMEM;

	options->non_block = 0;
	options->net_timeout = -1; 		/* system default */

	*netdata = options;
	*netapi = &_tcp_net_api;
	return 0;
}

int _tcp_set_option(void *netdata, const char *name, const char *value)
{
	struct _tcp_options *options = netdata;
	
	if ( strcasecmp(name, "tcp_non_block") == 0 ) {
		if ( value[0] == 't' || value[0] == 'T' || value[0] == '1' || value[0] == 'y' || value[0] == 'Y')
			options->non_block = 1;
		else
			options->non_block = 0;
	} 
	else if ( strcasecmp(name, "tcp_timeout") == 0 )
		options->net_timeout = atoi(value);
	else
		return EINVAL;
	return 0;
}

int _tcp_destroy(void **netdata)
{
	struct _tcp_options *options = *netdata;
	
	free(options);
	*netdata = NULL;
	return 0;
}

