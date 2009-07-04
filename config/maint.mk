# This file is part of GNU Mailutils.
# Copyright (C) 2009 Free Software Foundation, Inc.
#
# Written by Sergey Poznyakoff
#
# GNU Mailutils is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GNU Mailutils is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Mailutils.  If not, see <http://www.gnu.org/licenses/>.

include Makefile
mailutils_config_CFLOW_INPUT=$(mailutils_config_SOURCES)
mailutils-config.cflow: $(mailutils_config_CFLOW_INPUT) Makefile
	cflow -o$@ $(CFLOW_FLAGS) $(DEFS) \
		$(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
		$(CPPFLAGS) \
	$(mailutils_config_CFLOW_INPUT)

flowgraph: mailutils-config.cflow
maintclean:
	rm -f mailutils-config.cflow
