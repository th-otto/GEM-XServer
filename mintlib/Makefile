# Makefile -- MiNTLib.
# Copyright (C) 1999, 2000 Guido Flohr <guido@freemint.de>
#
# This file is part of the MiNTLib project, and may only be used
# modified and distributed under the terms of the MiNTLib project
# license, COPYMINT.  By continuing to use, modify, or distribute
# this file you indicate that you have read the license and
# understand and accept it fully.

MAKEFLAGS = -r
SHELL = /bin/sh

srcdir = .
top_srcdir = .

include $(top_srcdir)/configvars

SUBDIRS = include startup conf crypt dirent gmp login mintlib misc posix \
  pwdgrp shadow signal socket stdiio stdio stdlib string sysvipc termios \
  time unix lib

include $(srcdir)/SRCFILES
include $(srcdir)/MISCFILES
include $(srcdir)/EXTRAFILES

default: all

include phony

all:: all-here all-recursive

all-here:

install: all-here install-recursive zoneswarning

uninstall: uninstall-recursive

install-lib: all install-lib-recursive

install-include: all install-include-recursive

install-man: all install-man-recursive

clean:: clean-recursive

bakclean: bakclean-recursive

checkclean: checkclean-recursive

dist-check:
	@echo "disabled in bundled version"

dist:
	@echo "disabled in bundled version"

distdir:
	@echo "disabled in bundled version"

bin-dist:
	@echo "disabled in bundled version"

bindistdir:
	@echo "disabled in bundled version"

install-recursive uninstall-recursive install-lib-recursive \
install-include-recursive install-man-recursive uninstall-lib-recursive \
uninstall-include-recursive uninstall-man-recursive:
	@echo "disabled in bundled version"

all-recursive clean-recursive bakclean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  target=`echo $@ | sed s/-recursive//`; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

help:
	@echo "This Makefile (and most of the Makefiles in the subdirectories)"
	@echo "supports the following main targets:"
	@echo 
	@echo "     help         show this help page (toplevel Makefile only)"
	@echo "     all          build everyting (default)"
	@echo 
	@echo "Please read the file INSTALL before running make.  Edit"
	@echo "the file configvars so that it matches the needs of "
	@echo "your site."

zonenames:
	@echo "disabled in bundled version"
	cd $(top_srcdir)/tz && $(MAKE) zonenames

zoneswarning:

check:
	@echo "disabled in bundled version"
