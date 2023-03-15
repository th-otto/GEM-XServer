# startup/Makefile -- MiNTLib.
# Copyright (C) 1999, 2000 Guido Flohr <guido@freemint.de>
#
# This file is part of the MiNTLib project, and may only be used
# modified and distributed under the terms of the MiNTLib project
# license, COPYMINT.  By continuing to use, modify, or distribute
# this file you indicate that you have read the license and
# understand and accept it fully.

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
subdir = startup

default: all

include $(top_srcdir)/configvars

STANDARD_MODULES = crt0.o gcrt0.o

ifdef WITH_SHARED_TEXT
SHARED_TEXT_MODULES = bcrt0.o bgcrt0.o
endif

OBJS = $(STANDARD_MODULES) $(SHARED_TEXT_MODULES)

libdir = $(prefix)/lib

ifdef WITH_SOFT_FP
SFPFLAG = -Dsfp004
endif

ASFLAGS = 

INCLUDES = -nostdinc -I$(top_srcdir)/include

COMPILE = $(CC) $(WARN) $(CFLAGS) $(INCLUDES) $(DEFS) $(SFPFLAG) $(ASFLAGS)

all-here: $(OBJS)

include $(top_srcdir)/phony

crt0.o: $(srcdir)/crt0.S
	$(COMPILE) -c $< -o $@

gcrt0.o: $(srcdir)/crt0.S
	$(COMPILE) -DGCRT0 -c $< -o $@

bcrt0.o: $(srcdir)/crt0.S
	$(COMPILE) -mbaserel -c $< -o $@

bgcrt0.o: $(srcdir)/crt0.S
	$(COMPILE) -DGCRT0 -mbaserel -c $< -o $@

include $(top_srcdir)/rules

install:

uninstall:
	
install-include:

install-man:

uninstall-include:

uninstall-man:

checkclean:

