COPTS	= -Wall -c --no-warn

OBJS	= ../objs/main.o ../objs/settings.o ../objs/bitmapwindow.o ../objs/iconview.o ../objs/iconmenu.o ../objs/iconmenu_prop.o ../objs/loadbitmap.o ../objs/imageitem.o ../objs/properties.o ../objs/bitmapscale.o ../objs/drives.o ../objs/coloredit.o ../objs/drives_settings.o 
EXE	= ../objs/desktop

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	@echo Building : $(EXE)
	@$(LL) $(OBJS) -latheos -o $@
	@echo Adding images to : $(EXE)
	@rescopy -r $(EXE) ./images/*.jpg ./images/*.png

install: $(EXE)
	cp -f $(EXE) /bin/$(EXE)
	strip --strip-all /bin/$(EXE)

clean:  
	-rm -f $(OBJS)
	-rm -f $(EXE)

dist:
	@echo Copying $(EXE) to $(DIST_DIR)/atheos/sys/bin
	@cp -f $(EXE) $(DIST_DIR)/atheos/sys/bin

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

