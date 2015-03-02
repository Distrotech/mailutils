# This file is part of GNU Mailutils.
# Copyright (C) 2009-2012, 2014-2015 Free Software Foundation, Inc.
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

CFLOW_FLAGS=-i^s --brief -v

MAINT_CLEAN_FILES=

define flowgraph_tmpl
MAINT_CLEAN_FILES += $(1).cflow
$(2)_CFLOW_INPUT=$$($(2)_OBJECTS:.$(3)=.c)
$(1).cflow: $$($(2)_CFLOW_INPUT) Makefile
	cflow -o$$@ $$(CFLOW_FLAGS) $$(DEFS) \
		$$(DEFAULT_INCLUDES) $$(INCLUDES) $$(AM_CPPFLAGS) \
		$$(CPPFLAGS) \
	$$($(2)_CFLOW_INPUT)
endef

$(foreach prog,$(bin_PROGRAMS) $(sbin_PROGRAMS),\
$(eval $(call flowgraph_tmpl,$(prog),$(subst -,_,$(prog)),$(OBJEXT))))

$(foreach prog,$(lib_LTLIBRARIES),\
$(eval $(call flowgraph_tmpl,$(prog),$(subst .,_,$(prog)),lo)))

all: flowgraph-recursive

flowgraph: $(foreach prog,$(bin_PROGRAMS) $(sbin_PROGRAMS) $(lib_LTLIBRARIES),$(prog).cflow)

maintclean:
	-test -n "$(MAINT_CLEAN_FILES)" && rm -f $(MAINT_CLEAN_FILES)

##

MAINT_MK = maint.mk

MAINT_RECURSIVE_TARGETS=flowgraph-recursive maintclean-recursive

$(MAINT_RECURSIVE_TARGETS):
	@cwd=`pwd`; \
	failcom='exit 1'; \
	for f in x $$MAKEFLAGS; do \
	  case $$f in \
	    *=* | --[!k]*);; \
	    *k*) failcom='fail=yes';; \
	  esac; \
	done; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  if test -f $$subdir/$(MAINT_MK); then \
	    makefile='$(MAINT_MK)'; \
	  else \
	    makefile="$$cwd/maint.mk"; \
	  fi; \
	  $(MAKE) -C $$subdir -f $$makefile $$local_target \
	     || eval $$failcom; \
	done; \
	test -z "$$fail"
