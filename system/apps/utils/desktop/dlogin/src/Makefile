COPTS	= -c --no-warn

OBJDIR	:= ../objs
OBJS	= ../objs/main.o ../objs/commonfuncs.o ../objs/mainwindow.o ../objs/colorbutton.o ../objs/imagebutton.o
EXE		= $(OBJDIR)/dlogin

all: $(OBJDIR) $(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(EXE): $(OBJS)
	@echo Building : $(EXE)
	@$(CXX) $(OBJS) -lsyllable -lcrypt -o $@
	@echo Adding images to : $(EXE)
	@rescopy -r $(EXE) ./images/*.gif ./images/*.png

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
	@$(CXX) $(COPTS) $< -o $@

../objs/commonfuncs.o : commonfuncs.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

../objs/mainwindow.o : mainwindow.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

../objs/colorbutton.o : colorbutton.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

../objs/imagebutton.o : imagebutton.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

