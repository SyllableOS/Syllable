COPTS   = -c -O2 -Wall -fexceptions

OBJS = mixer.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

EXE := Mixer

#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@

all :	objs $(EXE) $(PLUGIN)

deps:

objs:
	mkdir objs

clean:
	-rm $(OBJDIR)/*
	-rm $(EXE) $(PLUGIN)

$(EXE): $(OBJS)
	g++ $(OBJS) -o $(EXE) -lsyllable -lmedia
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png

install: objs $(EXE)
	cp -f $(EXE) /system/bin/$(EXE)

dist: objs $(EXE)
	cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin/$(EXE)

