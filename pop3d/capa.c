/* GNU mailutils - a suite of utilities for electronic mail
   Copyright (C) 1999, 2001 Free Software Foundation, Inc.

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

#include "pop3d.h"

/*
  The CAPA Command

  The POP3 CAPA command returns a list of capabilities supported by the
  POP3 server.  It is available in both the AUTHORIZATION and
  TRANSACTION states.

  Capabilities available in the AUTHORIZATION state MUST be announced
  in both states.  */
int
pop3d_capa (const char *arg)
{
  if (strlen (arg) != 0)
    return ERR_BAD_ARGS;

  if (state != AUTHORIZATION && state != TRANSACTION)
    return ERR_WRONG_STATE;

  fprintf (ofile, "+OK Capability list follows\r\n");
  fprintf (ofile, "TOP\r\n");
  fprintf (ofile, "USER\r\n");
  fprintf (ofile, "UIDL\r\n");
  fprintf (ofile, "RESP-CODES\r\n");
  fprintf (ofile, "PIPELINING\r\n");
  /* FIXME: This can be Implemented by setting an header field on the
     message.  */
  /*fprintf (ofile, "EXPIRE NEVER\r\n"); */
  if (state == TRANSACTION)	/* let's not advertise to just anyone */
    fprintf (ofile, "IMPLEMENTATION %s %s\r\n", IMPL, VERSION);
  fprintf (ofile, ".\r\n");
  return OK;
}
