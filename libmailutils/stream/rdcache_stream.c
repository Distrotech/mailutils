/* GNU Mailutils -- a suite of utilities for electronic mail
   Copyright (C) 2009-2012 Free Software Foundation, Inc.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <mailutils/types.h>
#include <mailutils/sys/rdcache_stream.h>

size_t mu_rdcache_stream_max_memory_size = 4096;

static int
rdcache_read (struct _mu_stream *str, char *buf, size_t size, size_t *pnbytes)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;
  int status = 0;
  size_t nbytes = 0;
  
  if (sp->offset < sp->size)
    {
      status = mu_stream_read (sp->cache, buf, size, &nbytes);
      if (status)
	return status;
      sp->offset += nbytes;
      sp->size += nbytes;
      buf += nbytes;
      size -= nbytes;
    }
  else if (sp->offset > sp->size)
    {
      size_t left = sp->offset - sp->size;
      status = mu_stream_seek (sp->cache, 0, MU_SEEK_END, NULL);
      if (status)
	return status;
      status = mu_stream_copy (sp->cache, sp->transport, left, NULL);
      if (status)
	return status;
      sp->size = sp->offset;
    }

  if (size)
    {
      size_t rdbytes;
      status = mu_stream_read (sp->transport, buf, size, &rdbytes);
      if (rdbytes)
	{
	  int rc;

	  sp->offset += rdbytes;
	  sp->size += rdbytes;
	  nbytes += rdbytes;
	  rc = mu_stream_write (sp->cache, buf, rdbytes, NULL);
	  if (rc)
	    {
	      if (status == 0)
		status = rc;
	    }
	}
    }
  if (pnbytes)
    *pnbytes = nbytes;
  return status;
}

static int
rdcache_size (struct _mu_stream *str, off_t *psize)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;
  *psize = sp->size;
  return 0;
}

static int
rdcache_seek (struct _mu_stream *str, mu_off_t off, mu_off_t *presult)
{ 
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;

  if (off < 0)
    return ESPIPE;

  if (off < sp->size)
    {
      int status = mu_stream_seek (sp->cache, off, MU_SEEK_SET, NULL);
      if (status)
	return status;
    }
  
  sp->offset = off;
  *presult = sp->offset;
  return 0;
}

static int
rdcache_wait (struct _mu_stream *str, int *pflags, struct timeval *tvp)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;
  return mu_stream_wait (sp->transport, pflags, tvp);
}

/* FIXME: Truncate? */

static int
rdcache_ioctl (struct _mu_stream *str, int code, int opcode, void *arg)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;

  switch (code)
    {
    case MU_IOCTL_TRANSPORT:
      if (!arg)
	return EINVAL;
      else
	{
	  mu_transport_t *ptrans = arg;

	  switch (opcode)
	    {
	    case MU_IOCTL_OP_GET:
	      ptrans[0] = (mu_transport_t) sp->transport;
	      ptrans[1] = NULL;
	      break;
	    case MU_IOCTL_OP_SET:
	      return ENOSYS;
	    default:
	      return EINVAL;
	    }
	}
      break;
	      
    case MU_IOCTL_TRANSPORT_BUFFER:
      if (!arg)
	return EINVAL;
      else
	{
	  struct mu_buffer_query *qp = arg;
	  if (qp->type != MU_TRANSPORT_INPUT || !sp->transport)
	    return EINVAL;
	  return mu_stream_ioctl (sp->transport, code, opcode, arg);
	}

    default:
      return ENOSYS;
    }
  return 0;
}

static int
rdcache_open (struct _mu_stream *str)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;
  return mu_stream_open (sp->transport);
}

static int
rdcache_close (struct _mu_stream *str)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;
  return mu_stream_close (sp->transport);
}

static void
rdcache_done (struct _mu_stream *str)
{
  struct _mu_rdcache_stream *sp = (struct _mu_rdcache_stream *) str;
  mu_stream_unref (sp->transport);
  mu_stream_unref (sp->cache);
}
  
int
mu_rdcache_stream_create (mu_stream_t *pstream, mu_stream_t transport,
			  int flags)
{
  struct _mu_rdcache_stream *sp;
  int rc;
  int sflags = MU_STREAM_READ | MU_STREAM_SEEK;

  if (flags & ~sflags)
    return EINVAL;
  
  sp = (struct _mu_rdcache_stream *)
          _mu_stream_create (sizeof (*sp), sflags | _MU_STR_OPEN);
  if (!sp)
    return ENOMEM;

  sp->stream.read = rdcache_read;
  sp->stream.open = rdcache_open;
  sp->stream.close = rdcache_close;
  sp->stream.done = rdcache_done;
  sp->stream.seek = rdcache_seek; 
  sp->stream.size = rdcache_size; 
  sp->stream.ctl = rdcache_ioctl;
  sp->stream.wait = rdcache_wait;

  mu_stream_ref (transport);
  sp->transport = transport;

  if ((rc = mu_memory_stream_create (&sp->cache, MU_STREAM_RDWR)))
    {
      mu_stream_destroy ((mu_stream_t*) &sp);
      return rc;
    }
  
  *pstream = (mu_stream_t) sp;
  return 0;
}

