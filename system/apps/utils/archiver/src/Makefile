OPTS   = -Wall -c --no-warn

CC	= g++
EXE	= ../objs/archiver

OBJDIR	:= ../objs


OBJS := ../objs/main.o ../objs/toolbar.o ../objs/archiver.o ../objs/new.o ../objs/options.o ../objs/exe.o ../objs/extract.o ../objs/statusbar.o ../objs/imageitem.o ../objs/loadbitmap.o 



$(EXE): $(OBJS)
	@echo Building : $(EXE)
	@g++ $(OBJS) -latheos -o $(EXE)
	@echo Adding images into : $(EXE)
	@rescopy -r ../objs/archiver ./images/*.png

deps:
	@echo Making $(OBJDIR) directory
	@mkdir objs
	
dist:
	@echo Making $(DIST_DIR)/Applications/Archiver
	@mkdir $(DIST_DIR)/Applications/Archiver
	@echo Making $(DIST_DIR)/Applications/Archiver/bin
	@mkdir $(DIST_DIR)/Applications/Archiver/bin
	@echo Coping $(EXE) to $(DIST_DIR)/Applications/Archiver/
	@cp -f ../objs/archiver $(DIST_DIR)/Applications/Archiver/

clean:
	@echo Removing all objects and executable.
	@rm $(OBJS)
	@rm $(EXE)

../objs/.cpp.o:
	$(CC) $(COPTS) $< -o $@

../objs/main.o : main.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/archiver.o : archiver.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/new.o : new.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/options.o : options.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/extract.o : extract.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/exe.o : exe.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/statusbar.o : statusbar.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/imageitem.o : imageitem.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/toolbar.o : toolbar.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@





