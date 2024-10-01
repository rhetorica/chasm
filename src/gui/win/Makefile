
include ../../Makefile.cnf

CFLAGS += -I../includes $(SDL_CFLAGS)

GUIWINOBJS = opencon.o

all: gui-win.a chasm-winicon.o

gui-win.a: $(GUIWINOBJS)
	$(AR) cru $@ $^
	$(RANLIB) $@

chasm-winicon.o:
	$(WINDRES) -o chasm-winicon.o chasm-winicon.rc

clean distclean:
	$(RM) *.o
