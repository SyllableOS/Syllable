COPTS   = -c -fexceptions

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
	$(CXX) $(COPTS) $< -o $@

all :	objs $(EXE) $(PLUGIN)

deps:

objs:
	mkdir objs

clean:
	-rm $(OBJDIR)/*
	-rm $(EXE) $(PLUGIN)

$(EXE): $(OBJS)
	$(CXX) $(OBJS) -o $(EXE) -lsyllable -lmedia
	strip --strip-all $(EXE)
	rescopy $(EXE) -r *.png

install: objs $(EXE)
	cp -f $(EXE) $(IMAGE)/system/bin/

dist: install

