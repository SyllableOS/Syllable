#Makefile for FileTypes
COPTS = -c

APPBIN = /atheos/Applications/Preferences/
OBJS = main.o application.o mainwindow.o newtype.o

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

all : objs FileTypes

objs:
	@mkdir -p objs

FileTypes: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o FileTypes -lsyllable
	@echo Adding resources...
	@rescopy FileTypes -r icon48x48.png icon24x24.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f FileTypes

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/Applications/Preferences/
	@cp FileTypes $(DIST_DIR)/atheos/Applications/Preferences//FileTypes

install: all
	@echo Installing...
	@mkdir -p /atheos/Applications/Preferences/
	@cp FileTypes /atheos/Applications/Preferences//FileTypes

