OPTS   = -c -Wall

EXE	= CharMap

OBJDIR	:= objs
INSTALLDIR := /Applications/CharMap

$(OBJDIR)/%.o : %.c
	$(CC) $(OPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(OPTS) $< -o $@

OBJS := ./objs/main.o ./objs/mainwin.o ./objs/charmap.o

$(EXE): objs $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	-rescopy -r $(EXE) ./images/*.png
deps:
	mkdir objs

clean:
	-rm $(OBJS)

install: $(EXE)
	-mkdir $(INSTALLDIR)
	cp $(EXE) $(INSTALLDIR)/$(EXE)

dist: $(EXE)
	-mkdir $(DIST_DIR)/atheos$(INSTALLDIR)
	cp $(EXE) $(DIST_DIR)/atheos$(INSTALLDIR)/$(EXE)

