CFLAGS	 = -Wall -c
CPPFLAGS = -Wall -c

OBJS	= main.o editview.o controlpanel.o iconview.o winicon.o bitmapscale.o
EXE	= iconedit

CC	= gcc
LL	= gcc

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

$(OBJDIR)/$(EXE) : $(OBJDIR) $(OBJS)
	$(LL) $(OBJS) -latheos -lstdc++ -lm -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

deps: $(OBJDIR) $(DEPS)

install: $(OBJDIR)/$(EXE)
	cp -f $(OBJDIR)/$(EXE) /bin/$(EXE)
	strip --strip-all /bin/$(EXE)

dist: $(OBJDIR)/$(EXE)
	cp -f $(OBJDIR)/$(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)
	strip --strip-all $(DIST_DIR)/atheos/sys/bin/$(EXE)

clean:
	-rm $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/$(EXE)

-include $(OBJDIR)/*.d

