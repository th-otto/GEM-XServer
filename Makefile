top_srcdir=.

include $(top_srcdir)/CONFIGVARS

all clean::
ifeq ($(WITH_INCLUDED_MINTLIB),yes)
	$(MAKE) -C mintlib $@
endif
	$(MAKE) -C src $@

install uninstall::
	$(MAKE) DESTDIR=$(DESTDIR) -C src $@
