COPTS = -Wall -c --no-warn

OBJS = ../objs/main.o ../objs/loadbitmap.o ../objs/colorbutton.o
EXE	= ../objs/dlogin

CC	= g++
LL	= g++

$(EXE) : $(OBJS)
	@echo Building : $(EXE)
	@$(LL) $(OBJS) -latheos -lcrypt -o $@
	@echo Adding images to : $(EXE)
	@rescopy -r $(EXE) ./images/*.gif
 


clean:
	-rm -f $(OBJS)
	-rm -f $(EXE)


dist:
	@echo Copying $(EXE) to $(DIST_DIR)/atheos/sys/bin
	@cp  -f $(EXE) $(DIST_DIR)/atheos/sys/bin


../objs/main.o : main.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

../objs/colorbutton.o : colorbutton.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@













