/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2000, 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "mu_scm.h"

struct mu_port
{
  stream_t stream;         /* Associated stream */
  int offset;              /* Current offset in the stream */
  SCM msg;                 /* Message the port belongs to */		
};

#define DEFAULT_BUF_SIZE 1024
#define MU_PORT(x) ((struct mu_port *) SCM_STREAM (x))

static void
mu_port_alloc_buffer (SCM port, size_t read_size, size_t write_size)
{
  scm_port *pt = SCM_PTAB_ENTRY (port);
  static char *s_mu_port_alloc_buffer = "mu_port_alloc_buffer";
  
  if (!read_size)
    read_size = DEFAULT_BUF_SIZE;
  if (!write_size)
    write_size = DEFAULT_BUF_SIZE;

  if (SCM_INPUT_PORT_P (port))
    {
      pt->read_buf = malloc (read_size);
      if (pt->read_buf == NULL)
	scm_memory_error (s_mu_port_alloc_buffer);
      pt->read_pos = pt->read_end = pt->read_buf;
      pt->read_buf_size = read_size;
    }
  else
    {
      pt->read_pos = pt->read_buf = pt->read_end = &pt->shortbuf;
      pt->read_buf_size = 1;
    }
  
  if (SCM_OUTPUT_PORT_P (port))
    {
      pt->write_buf = malloc (write_size);
      if (pt->write_buf == NULL)
	scm_memory_error (s_mu_port_alloc_buffer);
      pt->write_pos = pt->write_buf;
      pt->write_buf_size = write_size;
      pt->write_end = pt->write_buf + pt->write_buf_size;
    }
  else
    {
      pt->write_buf = pt->write_pos = &pt->shortbuf;
      pt->write_buf_size = 1;
    }
  
  SCM_SET_CELL_WORD_0 (port, SCM_CELL_WORD_0 (port) & ~SCM_BUF0);
}

static long scm_tc16_smuport;

SCM
mu_port_make_from_stream (SCM msg, stream_t stream, long mode)
{
  struct mu_port *mp;
  SCM port;
  scm_port *pt;
  
  mp = scm_must_malloc (sizeof (struct mu_port), "mu-port");
  mp->msg = msg;
  mp->stream = stream;
  mp->offset = 0;

  SCM_NEWCELL (port);
  SCM_DEFER_INTS;
  pt = scm_add_to_port_table (port);
  SCM_SETPTAB_ENTRY (port, pt);
  pt->rw_random = stream_is_seekable (stream);
  SCM_SET_CELL_TYPE (port, (scm_tc16_smuport | mode));
  SCM_SETSTREAM (port, mp);
  mu_port_alloc_buffer (port, 0, 0);
  SCM_ALLOW_INTS;
  /*  SCM_PTAB_ENTRY (port)->file_name = "name";FIXME*/
  return port;
}

static SCM
mu_port_mark (SCM port)
{
  struct mu_port *mp = MU_PORT (port);
  return mp->msg;
}

static void
mu_port_flush (SCM port)
{
  struct mu_port *mp = MU_PORT (port);
  scm_port *pt = SCM_PTAB_ENTRY (port);
  int wrsize = pt->write_pos - pt->write_buf;
  size_t n;
  
  if (wrsize)
    {
      if (stream_write (mp->stream, pt->write_buf, wrsize, mp->offset, &n))
	return;
      mp->offset += n;
    }
  pt->write_pos = pt->write_buf;
  pt->rw_active = SCM_PORT_NEITHER;
}

static int
mu_port_close (SCM port)
{
  struct mu_port *mp = MU_PORT (port);
  scm_port *pt = SCM_PTAB_ENTRY (port);

  mu_port_flush (port);
  if (pt->read_buf != &pt->shortbuf)
    free (pt->read_buf);
  if (pt->write_buf != &pt->shortbuf)
    free (pt->write_buf);
  free (mp);
  return 0;
}

static scm_sizet
mu_port_free (SCM port)
{
  mu_port_close (port);
  return sizeof (struct mu_port); /*FIXME: approximate */
}

static int
mu_port_fill_input (SCM port)
{
  struct mu_port *mp = MU_PORT (port);
  scm_port *pt = SCM_PTAB_ENTRY (port);
  size_t nread = 0;
  
  if (stream_read (mp->stream, pt->read_buf, pt->read_buf_size,
		   mp->offset, &nread))
      scm_syserror ("mu_port_fill_input");

  if (nread == 0)
    return EOF;

  mp->offset += nread;
  pt->read_pos = pt->read_buf;
  pt->read_end = pt->read_buf + nread;
  return *pt->read_buf;
}

static void
mu_port_write (SCM port, const void *data, size_t size)
{
  scm_port *pt = SCM_PTAB_ENTRY (port);
  size_t remaining = size;
  char *input = (char*) data;
  
  while (remaining > 0)
    {
      int space = pt->write_end - pt->write_pos;
      int write_len = (remaining > space) ? space : remaining;
      
      memcpy (pt->write_pos, input, write_len);
      pt->write_pos += write_len;
      remaining -= write_len;
      input += write_len;
      if (write_len == space)
	mu_port_flush (port);
    }
}

/* Perform the synchronisation required for switching from input to
   output on the port.
   Clear the read buffer and adjust the file position for unread bytes. */
static void
mu_port_end_input (SCM port, int offset)
{
  struct mu_port *mp = MU_PORT (port);
  scm_port *pt = SCM_PTAB_ENTRY (port);
  int delta = pt->read_end - pt->read_pos;
  
  offset += delta;

  if (offset > 0)
    {
      pt->read_pos = pt->read_end;
      mp->offset -= delta;
    }
  pt->rw_active = SCM_PORT_NEITHER;
}

static off_t
mu_port_seek (SCM port, off_t offset, int whence)
{
  struct mu_port *mp = MU_PORT (port);
  scm_port *pt = SCM_PTAB_ENTRY (port);
  off_t size = 0;
  
  if (whence == SEEK_CUR && offset == 0)
    return mp->offset;

  if (pt->rw_active == SCM_PORT_WRITE)
    {
      mu_port_flush (port);
    }
  else if (pt->rw_active == SCM_PORT_READ)
    {
      scm_end_input (port);
    }

  stream_size (mp->stream, &size);
  switch (whence)
    {
    case SEEK_SET:
      break;
    case SEEK_CUR:
      offset += mp->offset;
      break;
    case SEEK_END:
      offset += size;
    }

  if (offset > size)
    return -1;
  mp->offset = offset;
  return offset;
}

static void
mu_port_truncate (SCM port, off_t length)
{
  struct mu_port *mp = MU_PORT (port);
  if (stream_truncate (mp->stream, length))
    scm_syserror ("stream_truncate");
}
  
static int
mu_port_print (SCM exp, SCM port, scm_print_state *pstate)
{
  struct mu_port *mp = MU_PORT (exp);
  off_t size = 0;
  
  scm_puts ("#<", port);
  scm_print_port_mode (exp, port);
  scm_puts ("mu-port", port);
  if (stream_size (mp->stream, &size) == 0)
    {
      char buffer[64];
      snprintf (buffer, sizeof (buffer), " %-5ld", size);
      scm_puts (buffer, port);
      scm_puts (" chars", port);
    }
  scm_putc ('>', port);
  return 1;
}
     
void
mu_scm_port_init ()
{
    scm_tc16_smuport = scm_make_port_type ("mu-port",
					   mu_port_fill_input, mu_port_write);
    scm_set_port_mark (scm_tc16_smuport, mu_port_mark);
    scm_set_port_free (scm_tc16_smuport, mu_port_free);
    scm_set_port_print (scm_tc16_smuport, mu_port_print);
    scm_set_port_flush (scm_tc16_smuport, mu_port_flush);
    scm_set_port_end_input (scm_tc16_smuport, mu_port_end_input);
    scm_set_port_close (scm_tc16_smuport, mu_port_close);
    scm_set_port_seek (scm_tc16_smuport, mu_port_seek);
    scm_set_port_truncate (scm_tc16_smuport, mu_port_truncate);
    /*    scm_set_port_input_waiting (scm_tc16_smuport, mu_port_input_waiting);*/
}
