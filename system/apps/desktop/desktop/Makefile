#Makefile for desktop
COPTS = -c -fexceptions -ffast-math

APPBIN = /system/bin
OBJS = main.o application.o desktop.o

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

all : objs desktop

objs:
	@mkdir -p objs

desktop: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o desktop -lsyllable
	@echo Adding resources...
	@rescopy desktop -r icon48x48.png background.jpg 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f desktop

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/bin
	@cp desktop $(DIST_DIR)/atheos/sys/bin/desktop

install: all
	@echo Installing...
	@mkdir -p /system/bin
	@cp desktop /system/bin/desktop

