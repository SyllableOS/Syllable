COPTS	= -Wall -c --no-warning

OBJDIR	:= ../objs
OBJS	= ../objs/main.o ../objs/settings.o ../objs/bitmapwindow.o ../objs/iconview.o ../objs/iconmenu.o ../objs/iconmenu_prop.o ../objs/loadbitmap.o ../objs/imageitem.o ../objs/properties.o ../objs/bitmapscale.o ../objs/drives.o ../objs/coloredit.o ../objs/drives_settings.o 
EXE		= $(OBJDIR)/desktop

CC	= g++
LL	= g++

all: $(OBJDIR) $(EXE)

$(OBJDIR):
	mkdir $(OBJDIR)

$(EXE): $(OBJS)
	@echo Building : $(EXE)
	@$(LL) $(OBJS) -lsyllable -o $@
	@echo Adding images to : $(EXE)
	@rescopy -r $(EXE) ./images/*.jpg ./images/*.png

install: all
	cp -f $(EXE) /bin/$(EXE)
	strip --strip-all /bin/$(EXE)

clean:  
	@echo Removing all objects and executables.
	@rm -f $(OBJS)
	@rm -f $(EXE)

dist: all
	@echo Copying $(EXE) to $(DIST_DIR)/atheos/sys/bin
	@cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin

astyle:
	@echo Cleaning up all of the source
	@astyle --style=ansi *.cpp
	@astyle --style=ansi *.h
	@rm -rf *.orig


../objs/main.o : main.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/bitmapwindow.o : bitmapwindow.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/settings.o : settings.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/iconview.o : iconview.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/iconmenu.o : iconmenu.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/iconmenu_prop.o : iconmenu_prop.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/imageitem.o : imageitem.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/properties.o : properties.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/bitmapscale.o : bitmapscale.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@
 
../objs/drives.o : drives.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/coloredit.o : coloredit.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/drives_settings.o : drives_settings.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@ 





