OPTS   = -c -Wall

CC	= gcc
EXE	= CharMap

OBJDIR	:= objs
INSTALLDIR := /Applications/CharMap

$(OBJDIR)/%.o : %.c
	$(CC) $(OPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(OPTS) $< -o $@

OBJS := ./objs/main.o ./objs/mainwin.o ./objs/charmap.o

$(EXE): objs $(OBJS)
	g++ $(OBJS) -lstdc++ -lsyllable -o $(EXE)
	-rescopy -r $(EXE) ./images/*.png
deps:
	mkdir objs

clean:
	-rm $(OBJS)

install: $(EXE)
	-mkdir $(INSTALLDIR)
	cp $(EXE) $(INSTALLDIR)/$(EXE)
