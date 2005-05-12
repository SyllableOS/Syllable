#Makefile for Dock
COPTS = -c

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
	@$(CXX) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@

all : objs Dock

objs:
	@mkdir -p objs

Dock: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o Dock -lsyllable
	@echo Adding resources...
	@rescopy Dock -r icon48x48.png icon24x24.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Dock

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	@cp Dock $(DIST_DIR)/atheos/Applications/Preferences/Dock

install: all
	@echo Installing...
	@mkdir -p /atheos/Applications/Preferences
	@cp Dock /atheos/Applications/Preferences/Dock

