COPTS	= -Wall -c --no-warn

OBJDIR	:= ../objs
OBJS	= ../objs/main.o ../objs/loadbitmap.o ../objs/colorbutton.o
EXE		= $(OBJDIR)/dlogin

CC	= g++
LL	= g++

all: $(OBJDIR) $(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(EXE): $(OBJS)
	@echo Building : $(EXE)
	@$(LL) $(OBJS) -lsyllable -lcrypt -o $@
	@echo Adding images to : $(EXE)
	@rescopy -r $(EXE) ./images/*.gif

clean:
	@echo Removing all objects and executables 
	@rm -f $(OBJS)
	@rm -f $(EXE)

dist: all
	@echo Copying $(EXE) to $(DIST_DIR)/atheos/sys/bin
	@cp  -f $(EXE) $(DIST_DIR)/atheos/sys/bin

astyle:
	@echo Cleaning up all of the source
	@astyle --style=ansi *.cpp
	@astyle --style=ansi *.h
	@rm -rf *.orig

../objs/main.o : main.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/colorbutton.o : colorbutton.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@
















