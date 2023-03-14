CROSS=m68k-atari-mint-

WARN    = -Wall -Wpointer-arith -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wshadow \
          -Werror #-Wcast-qual
CCOPTS  = -D_TRACE -g -O2 -fstrength-reduce #-DNODEBUG
CCFLAGS = -funsigned-char -fomit-frame-pointer -m68030 \
          -I. -I../include \
          $(CCOPTS) $(WARN)

#SHELL  = /bin/ash
#TMPDIR = /ram/tmp

CC  = $(CROSS)gcc
CXX = $(CROSS)g++ #-pipe
LD  = $(CC) -g
AR  = $(CROSS)ar rcs

PRG    = X.app
OBJDIR = ../obj
LIBS   = -lgem -liio
DEPEND = $(OBJDIR)/depends

LDFLAGS = $(CCOPTS)


$(OBJDIR)/%.o: %.c ; rm -f $(PRG) ; $(CC) $(CCFLAGS) -D_$(<:.c=)_ -c $< -o $@


ALL: $(PRG)

SRC = Atom.c Cursor.c Pointer.c Property.c Prop_ICCC.c Request.c \
      clnt.c clnt_swap.c colormap.c drawable.c draw_img.c \
      event.c font.c font_init.c fontable.c gcontext.c grph.c keyboard.c \
      main.c pixmap.c pixm_mono.c selection.c server.c \
      window.c wind_draw.c wind_grab.c wind_pntr.c wind_save.c wind_util.c \
      wmgr.c wmgr_draw.c wmgr_prog.c x_gem.c x_printf.c xrsc.c

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)


$(PRG): $(OBJDIR) $(OBJ) Makefile config.c ../include/version
	@expr "0`cat -s $(OBJDIR)/build`" + 1 > $(OBJDIR)/build
	@$(CC) $(CCFLAGS) -c config.c \
	       -DVERSION=\"`cat -s ../include/version`\" \
	       -DBUILD=\"`cat -s $(OBJDIR)/build`\" \
	       -o $(OBJDIR)/config.o
	$(LD) $(LDFLAGS) -o $@ $(OBJ) $(OBJDIR)/config.o $(LIBS)


$(OBJDIR):
	@echo "*** Prepare obj-Directory ***"
	@mkdir $(OBJDIR)
	@echo "0" > $(OBJDIR)/build
	@echo "Create dependencies ..."
	@$(MAKE) $(DPND)
	@echo "*** Make $(PRG) ***"


clean:
	rm -f *.% *.[bB][aA][kK] ../include/*.%
	rm -f $(OBJDIR)/*.o

#_______________________________________________________________________________
DPND = $(SRC:.c=.d)
depend: __udpnd Makefile $(DPND)
	@echo

__udpnd: ; rm -f $(DEPEND)

$(DPND):
	@echo -n " <$(@:%.d=%)> "
	@$(CC) -pipe $(CCFLAGS: $(WARN)=) -M $(@:%.d=%.c) -D_$(@:.d=)_ \
	 | sed "s,^\(.*\)\.o:,$(OBJDIR)/\1.o:," >> $(DEPEND)

-include $(OBJDIR)/depends
