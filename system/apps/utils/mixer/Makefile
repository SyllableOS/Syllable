COPTS   = -c -O2 -Wall -fexceptions

OBJS = app.o window.o view.o channel.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

EXE := mixer

#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs mixer

deps:

objs:
	mkdir objs

clean:
	-rm $(OBJDIR)/*
	-rm $(EXE)

mixer: $(OBJS)
	g++ $(OBJS) -o $(EXE) -latheos

install: objs mixer
	cp -f $(EXE) $(DEVBIN)/$(EXE)

dist: objs mixer
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

