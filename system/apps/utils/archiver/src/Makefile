OPTS   = -Wall -c --no-warn

EXE	= ../objs/Archiver

OBJDIR	:= ../objs


OBJS := ../objs/main.o ../objs/toolbar.o ../objs/archiver.o ../objs/new.o ../objs/options.o ../objs/exe.o ../objs/extract.o ../objs/statusbar.o ../objs/loadbitmap.o ../objs/settings.o ../objs/flowlayout.o



$(EXE): $(OBJS)
	@echo Building : $(EXE)
	@$(CXX) $(OBJS) -lsyllable -o $(EXE)
	@echo Adding images into : $(EXE)
	@rescopy -r $(EXE) ./images/*.png

deps:
	@echo Making $(OBJDIR) directory
	@mkdir $(OBJDIR)
	
dist:
	@echo Making $(DIST_DIR)/atheos/Applications/Archiver
	@mkdir $(DIST_DIR)/atheos/Applications/Archiver
	@echo Making $(DIST_DIR)/atheos/Applications/Archiver/bin
	@mkdir $(DIST_DIR)/atheos/Applications/Archiver/bin
	@echo Coping $(EXE) to $(DIST_DIR)/atheos/Applications/Archiver/
	@cp -f $(EXE) $(DIST_DIR)/atheos/Applications/Archiver/

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

../objs/loadbitmap.o : loadbitmap.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/toolbar.o : toolbar.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/settings.o : settings.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

../objs/flowlayout.o : flowlayout.cpp
	@echo Compiling : $<
	@$(CC) $(OPTS) $< -o $@

