#Makefile for FileBrowser
COPTS = -Wall -c -O4 -pipe -fexceptions -march=i586 -mcpu=i586 -ffast-math -fomit-frame-pointer

APPBIN = /system/bin
OBJS = main.o application.o mainwindow.o toolbar.o

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

all : objs FileBrowser

objs:
	@mkdir -p objs

FileBrowser: $(OBJS)
	@echo Linking...
	@gcc $(OBJS) -o FileBrowser  -lstdc++ -lsyllable
	@echo Adding resources...
	@rescopy FileBrowser -r home.png up.png breaker.png view.png icon24x24.png icon48x48.png reload.png back.png 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f FileBrowser

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/sys/bin
	@cp FileBrowser $(DIST_DIR)/atheos/sys/bin/FileBrowser

install: all
	@echo Installing...
	@mkdir -p /system/bin
	@cp FileBrowser /system/bin/FileBrowser

