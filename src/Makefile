top_srcdir=..
include $(top_srcdir)/CONFIGVARS

CCOPTS += -funsigned-char #-DNODEBUG
CFLAGS += -I. -I$(top_srcdir)/include
WARN += -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wshadow -Wpointer-arith
ifeq ($(WITH_INCLUDED_MINTLIB),yes)
GCCINCLUDE := $(shell $(CC) -print-file-name=include)
CFLAGS += -nostdinc -I$(top_srcdir)/mintlib/include -I$(GCCINCLUDE) -I$(GCCINCLUDE)-fixed
endif

#SHELL  = /bin/ash
#TMPDIR = /ram/tmp

PRG    = X.app
LIBS   = -lgem -liio

LDFLAGS = $(CCOPTS) -Wl,-stack,128k -s

all:: $(PRG)

SRCS = Atom.c Cursor.c Pointer.c Property.c Prop_ICCC.c Request.c \
      clnt.c clnt_swap.c colormap.c drawable.c draw_img.c \
      event.c font.c font_init.c fontable.c gcontext.c grph.c keyboard.c \
      main.c pixmap.c pixm_mono.c selection.c server.c \
      window.c wind_draw.c wind_grab.c wind_pntr.c wind_save.c wind_util.c \
      wmgr.c wmgr_draw.c wmgr_prog.c x_gem.c x_printf.c xrsc.c \
      config.c version.c


OBJS = $(SRCS:.c=.o)

DEP_FILES := $(addprefix .deps/,$(patsubst %.o,%.P,$(OBJS)))


$(PRG): $(OBJS) Makefile
ifeq ($(WITH_INCLUDED_MINTLIB),yes)
	$(AM_V_LD)$(LD) -nostdlib $(top_srcdir)/mintlib/startup/crt0.o $(LDFLAGS) -o $@ $(OBJS) \
	-L$(top_srcdir)/mintlib/lib \
	-lgem -lgcc -liio -lc -lgcc
else
	$(AM_V_LD)$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
endif

$(OBJS): Makefile

clean::
	rm -f $(PRG)

.SUFFIXES:
.SUFFIXES: .c .o .S .s

.c.o:
	$(AM_V_CC)$(CC) $(CFLAGS) -Wp,-MD,.deps/$(@:.o=.P) -c $< -o $@

.S.o:
	$(AM_V_AS)$(CC) $(CFLAGS) -Wp,-MD,.deps/$(@:.o=.P) -c $< -o $@

.s.o:
	$(AM_V_AS)$(CC) $(CFLAGS) -Wp,-MD,.deps/$(@:.o=.P) -c $< -o $@

