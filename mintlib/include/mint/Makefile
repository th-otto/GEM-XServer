# include/mint/Makefile -- MiNTLib.
# Copyright (C) 1999 Guido Flohr <gufl0000@stud.uni-sb.de>
#
# This file is part of the MiNTLib project, and may only be used
# modified and distributed under the terms of the MiNTLib project
# license, COPYMINT.  By continuing to use, modify, or distribute
# this file you indicate that you have read the license and
# understand and accept it fully.

SHELL = /bin/sh

srcdir = .
top_srcdir = ../..
csrcdir = $(top_srcdir)/src
subdir = mint

SUBDIRS = arch

default: all

include $(top_srcdir)/configvars

# Make install-libs work.
LIBS =

all-here:

include $(top_srcdir)/phony

include $(top_srcdir)/rules

includefiles = $(filter %.h, $(MISCFILES))

install-lib:

install-man:

uninstall-lib:

uninstall-man:
