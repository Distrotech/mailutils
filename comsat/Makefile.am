## Process this file with GNU Automake to create Makefile.in

##   Copyright (C) 2001, 2002, 2007 Free Software Foundation, Inc.
##
##   GNU Mailutils is free software; you can redistribute it and/or
##   modify it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 3, or (at
##   your option) any later version.
##
##   GNU Mailutils is distributed in the hope that it will be useful, but
##   WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
##   General Public License for more details.
##
##   You should have received a copy of the GNU General Public License
##   along with GNU Mailutils; if not, write to the Free Software
##   Foundation, Inc.  51 Franklin Street, Fifth Floor, Boston, MA
##   02110-1301 USA

INCLUDES = @MU_COMMON_INCLUDES@ 

sbin_PROGRAMS = comsatd

comsatd_SOURCES = action.c cfg.c comsat.c comsat.h

comsatd_LDADD = \
 ../lib/libmuaux.la\
 ${MU_LIB_MBOX}\
 ${MU_LIB_MAILUTILS}\
 @MU_COMMON_LIBRARIES@

CFLOW_FLAGS=-i^s --brief -v
comsatd_CFLOW_INPUT=$(comsatd_OBJECTS:.$(OBJEXT)=.c)
comsatd.cflow: $(comsatd_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
	$(comsatd_CFLOW_INPUT)
