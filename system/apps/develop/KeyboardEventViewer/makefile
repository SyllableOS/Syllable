#Makefile for KeyboardEventViewer
COPTS = -Wall -c -O2 -fexceptions

APPBIN = /Applications/sIDE
OBJS = main.o application.o mainwindow.o view.o common.o

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

all : translations objs "KeyboardEventViewer"


translations:

objs:
	@mkdir -p objs

"KeyboardEventViewer": $(OBJS)
	@echo Linking...
	@gcc $(OBJS) -o "KeyboardEventViewer"  -lsyllable -lstdc++
	@echo Adding resources...
	@rescopy "KeyboardEventViewer" -r icon48x48.png 
	@addattrib "KeyboardEventViewer" os::Category Development

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f "KeyboardEventViewer"

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/Applications/sIDE
	@cp "KeyboardEventViewer" "$(DIST_DIR)/Applications/sIDE/KeyboardEventViewer"

install: all
	@echo Installing...
	@mkdir -p /Applications/sIDE
	@cp "KeyboardEventViewer" "/Applications/sIDE/KeyboardEventViewer"

