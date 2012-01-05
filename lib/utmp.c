/* utmp.c -- Replacements for {set,get,end}utmp functions

Copyright (C) 2002, 2009-2012 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <sys/types.h> 
#include <sys/time.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <confpaths.h>

static char *utmp_name = PATH_UTMP;
static int fd = -1;
static struct utmp ut;

void
endutent ()
{
  if (fd > 0)
    close (fd);
  fd = -1;
}

void
setutent ()
{
  endutent ();
  if ((fd = open (utmp_name, O_RDWR)) < 0
      && ((fd = open (utmp_name, O_RDONLY)) < 0))
    perror ("setutent: Can't open utmp file");
}

struct utmp *
getutent ()
{
  if (fd < 0)
    setutent ();

  if (fd < 0 || read (fd, &ut, sizeof ut) != sizeof ut)
    return NULL;

  return &ut;
}
