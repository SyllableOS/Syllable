COPTS	= -Wall -c

OBJS	= ../objs/main.o ../objs/mainapp.o ../objs/appwindow.o ../objs/bitmapview.o ../objs/loadbitmap.o ../objs/statusbar.o ../objs/settingswindow.o
EXE	= ../objs/AView

CC	= g++
LL	= g++

$(EXE): deps $(OBJS)
	@echo Building : $(EXE)
	@g++ $(OBJS) -lsyllable -o $(EXE)
	@echo Adding images into : $(EXE)
	@rescopy -r ../objs/aview ./images/*.png

clean:
	@echo Removing all objects and the executable
	@rm $(OBJS)
	@-rm $(EXE)

deps:	
	mkdir -p ../objs

dist: $(EXE)
	mkdir $(DIST_DIR)/atheos/Applications/AView
	cp -f  $(EXE) $(DIST_DIR)/atheos/Applications/AView/

astyle:
	@echo Cleaning up all the source
	@astyle --style=ansi  *.cpp *.h
	@rm -rf *.orig

.cpp.o:
	$(CC) $(COPTS) $< -o $@

../objs/main.o : main.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/mainapp.o : mainapp.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/appwindow.o : appwindow.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/statusbar.o : statusbar.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/bitmapview.o : bitmapview.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/settingswindow.o: settingswindow.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

