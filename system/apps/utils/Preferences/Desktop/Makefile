#Makefile for Desktop
COPTS = -Wall -c

APPBIN = /atheos/Applications/Preferences
OBJS = Prefs.o

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@

all : objs Desktop

objs:
	@mkdir -p objs

Desktop: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o Desktop -lsyllable
	@echo Adding resources...
	@rescopy Desktop -r icon48x48.png icon24x24.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Desktop

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	@cp Desktop $(DIST_DIR)/atheos/Applications/Preferences/Desktop

install: all
	@echo Installing...
	@mkdir -p /atheos/Applications/Preferences
	@cp Desktop /atheos/Applications/Preferences/Desktop

