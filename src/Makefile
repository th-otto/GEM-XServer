CROSS=m68k-atari-mint-

AM_DEFAULT_VERBOSITY = 0

AM_V_CC = $(am__v_CC_$(V))
am__v_CC_ = $(am__v_CC_$(AM_DEFAULT_VERBOSITY))
am__v_CC_0 = @echo "  CC    " $@;
AM_V_LD = $(am__v_LD_$(V))
am__v_LD_ = $(am__v_LD_$(AM_DEFAULT_VERBOSITY))
am__v_LD_0 = @echo "  LD    " $@;
AM_V_AR = $(am__v_AR_$(V))
am__v_AR_ = $(am__v_AR_$(AM_DEFAULT_VERBOSITY))
am__v_AR_0 = @echo "  AR    " $@;
AM_V_RANLIB = $(am__v_RANLIB_$(V))
am__v_RANLIB_ = $(am__v_RANLIB_$(AM_DEFAULT_VERBOSITY))
am__v_RANLIB_0 = @echo "  RANLIB" $@;


WARN    = -Wall -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wshadow \
          -Werror #-Wcast-qual
CCOPTS  = -O2 -fomit-frame-pointer -fno-strict-aliasing -m68030 #-DNODEBUG
CCFLAGS = -funsigned-char \
          -I. -I../include \
          $(CCOPTS) $(WARN)

#SHELL  = /bin/ash
#TMPDIR = /ram/tmp

CC  = $(CROSS)gcc
CXX = $(CROSS)g++ #-pipe
LD  = $(CC) -g
AR  = $(CROSS)ar rcs

PRG    = X.app
LIBS   = -lgem -liio

LDFLAGS = $(CCOPTS) -Wl,-stack,128k -s

.c.o:
	$(AM_V_CC)$(CC) $(CCFLAGS) -Wp,-MD,.deps/$(@:.o=.P) -D_$(<:.c=)_ -c $< -o $@


all: $(PRG)

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
	$(AM_V_LD)$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS): Makefile

clean::
	rm -f *.[bB][aA][kK] *.o $(PRG)
	rm -rf .deps

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES) /dev/null
