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
#include <errno.h>

#include <net0.h>

static struct _netregistrar _netreg[1] = { "tcp", _tcp_create, _tcp_set_option, _tcp_destroy };

int net_api_create(net_t *net, net_t parent, const char *type)
{
	net_t n;
	int i, napis, ret = 0;
	
	if ( net == NULL || type == NULL ) 
		return EINVAL;

	*net = NULL;		

	if ( ( n = calloc(1, sizeof(*n)) ) == NULL )
		return ENOMEM;
	napis = sizeof(_netreg) / sizeof(_netreg[0]); 
	for( i = 0; i < napis; i++ ) {
		if ( strcasecmp(_netreg[i].type, type) == 0 )
			break;
	}
	if ( i == napis )
		return ENOTSUP;
	if ( ret = ( _netreg[i].create(&(n->data), &(n->api)) ) != 0 )
		free(n);
	n->parent = parent;
	n->net_reg = &_netreg[i];
	*net = n;
	return 0;
}

int net_api_set_option(net_t net, const char *name, const char *value)
{
	if ( net && name && value ) 
		return net->net_reg->set_option(net->data, name, value);
	return EINVAL;	
}

int net_api_destroy(net_t *net)
{
	net_t n;
	if ( net == NULL || *net == NULL ) 
		return EINVAL;

	n = *net;			
	n->net_reg->destroy(&n->data);	
	free(n);
	*net = NULL;		
	return 0;	
}

int net_new(net_t net, netinstance_t *inst)
{
	netinstance_t netinst;
	int ret = 0;
	
	if ( net == NULL || inst == NULL )
		return EINVAL;
	
	*inst = NULL;
		
	if ( ( netinst = calloc(1, sizeof(*netinst)) ) == NULL ) 
		return ENOMEM;
	netinst->api = net->api;
	if ( ( ret = net->api->new(net->data, net->parent, &(netinst->data)) ) != 0 ) {
		free(netinst);
		return ret;
	}
	*inst = netinst;
	return 0;
}

int net_connect(netinstance_t inst, const char *host, int port)
{
	if ( inst == NULL || host == NULL )
		return EINVAL;
	
	return inst->api->connect(inst->data, host, port);
}

int net_get_stream(netinstance_t inst, stream_t *iostr)
{
	if ( inst == NULL || iostr == NULL )
		return EINVAL;
	
	return inst->api->get_stream(inst->data, iostr);
}

int net_close(netinstance_t inst)
{
	if ( inst == NULL )
		return EINVAL;
	
	return inst->api->close(inst->data);
}

int net_free(netinstance_t *pinst)
{
	int ret;
	netinstance_t inst;
	
	if ( pinst == NULL || *pinst == NULL )
		return EINVAL;
	
	inst = *pinst;
	ret = inst->api->free(&(inst->data));
	free(inst);
	*pinst = NULL;
	return ret;
}
