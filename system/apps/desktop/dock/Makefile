#Makefile for Dock
COPTS = -Wall -c -O3 -pipe -fexceptions -ffast-math -fomit-frame-pointer

APPBIN = /system/bin
OBJS = Dock.o DirMenu.o

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

all : objs Dock

objs:
	@mkdir -p objs

Dock: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o Dock -lsyllable
	@echo Adding resources...
	@rescopy Dock -r logout.png syllable.png window.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Dock

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/bin
	@cp Dock $(DIST_DIR)/atheos/sys/bin/Dock

install: all
	@echo Installing...
	@mkdir -p /system/bin
	@cp Dock /system/bin/Dock

