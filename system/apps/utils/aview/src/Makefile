COPTS	= -Wall -c -o3

OBJS	= ../objs/main.o ../objs/mainapp.o ../objs/appwindow.o ../objs/bitmapview.o ../objs/loadbitmap.o ../objs/statusbar.o ../objs/settingswindow.o
EXE	= ../objs/aview

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	@echo Building : $(EXE)
	@g++ $(OBJS) -latheos -o $(EXE)
	@echo Adding images into : $(EXE)
	@rescopy -r ../objs/aview ./images/*.png

clean:
	@echo Removing all objects and the executable
	@rm $(OBJS)
	@-rm $(EXE)

deps:	
	@echo Making $(OBJDIR) directory
	@mkdir ../objs

dist:
	mkdir $(DIST_DIR)/Applications/AView
	mkdir $(DIST_DIR)/Applications/AView/bin
	cp -f  $(EXE) $(DIST_DIR)/Applications/AView/bin/$(EXE)

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












