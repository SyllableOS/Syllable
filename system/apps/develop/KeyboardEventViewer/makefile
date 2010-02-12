# Makefile for KeyboardEventViewer

COPTS = -Wall -c -O2 -fexceptions

APPBIN = /applications/sIDE
OBJS = main.o application.o mainwindow.o view.o common.o

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

all : translations objs "KeyboardEventViewer"


translations:

objs:
	@mkdir -p objs

"KeyboardEventViewer": $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o "KeyboardEventViewer" -lsyllable
	@echo Adding resources...
	@rescopy "KeyboardEventViewer" -r icon48x48.png 
	@addattrib "KeyboardEventViewer" os::Category Development

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f "KeyboardEventViewer"

deps:

install: all
	@echo Installing...
	@mkdir -p $(IMAGE)$(APPBIN)
	@cp "KeyboardEventViewer" "$(IMAGE)$(APPBIN)/KeyboardEventViewer"

dist: install
