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

#ifndef _REGISTRAR_H
#define _REGISTRAR_H

#include <sys/types.h>

#include <url.h>
#include <mailbox.h>

#ifndef __P
# ifdef __STDC__
#  define __P(args) args
# else
#  define __P(args) ()
# endif
#endif /*__P */

#ifdef _cpluscplus
extern "C" {
#endif

struct url_registrar
{
  char *scheme;
  int  (*_init)     __P ((url_t *, const char * name));
  void (*_destroy)  __P ((url_t *));
};

struct mailbox_registrar
{
  char *name;
  int  (*_init)    __P ((mailbox_t *, const char *name));
  void (*_destroy) __P ((mailbox_t *));
};

struct _registrar;

typedef struct _registrar* registrar_t;

/* mailbox registration */
extern int registrar_add    __P ((struct url_registrar *ureg,
				  struct mailbox_registrar *mreg, int *id));
extern int registrar_remove __P ((int id));
extern int registrar_get  __P ((int id, struct url_registrar **ureg,
				struct mailbox_registrar **mreg));
extern int registrar_list __P ((struct url_registrar **ureg,
			   struct mailbox_registrar **mreg,
			   int *id, registrar_t *reg));
#ifdef _cpluscplus
}
#endif

#endif /* _REGISTRAR_H */
