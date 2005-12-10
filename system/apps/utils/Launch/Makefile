#Makefile for Launch
COPTS = -c -fexceptions -ffast-math

APPBIN = /Applications/
OBJS = main.o application.o commonfuncs.o execute.o mainwindow.o

EXE = Launch

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@

all : objs $(EXE)

objs:
	@mkdir -p objs

$(EXE): $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o $(EXE) -lsyllable
	@echo Adding resources...
	@rescopy $(EXE) -r images/*.png resources/*.catalog
	@rescopy $(EXE) -a da/Launch.catalog=resources/da/Launch.catalog
	@rescopy $(EXE) -a nl/Launch.catalog=resources/nl/Launch.catalog
	@rescopy $(EXE) -a pl/Launch.catalog=resources/pl/Launch.catalog
	@rescopy $(EXE) -a sv/Launch.catalog=resources/sv/Launch.catalog

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f $(EXE)

deps:

dist: all
	@echo Distribution...
	@cp $(EXE) $(DIST_DIR)/atheos/Applications/

install: all
	@echo Installing...
	@cp $(EXE) /Applications/

