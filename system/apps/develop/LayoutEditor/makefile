#Makefile for LayoutEditor
COPTS = -Wall -c -fexceptions

APPBIN = /Applications/sIDE
OBJS = AppWindow.o EditList.o EditString.o PropertyRow.o WidgetsControls.o WidgetsImage.o WidgetsList.o WidgetsMenu.o WidgetsNodes.o WidgetsUtils.o application.o main.o mainwindow.o

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

all : translations objs "LayoutEditor"

translations:

objs:
	@mkdir -p objs

"LayoutEditor": $(OBJS)
	@echo Linking...
	@gcc $(OBJS) -o "LayoutEditor"  -lsyllable -lstdc++
	@echo Adding resources...
	@rescopy "LayoutEditor" -r images/breaker.png images/open.png images/save.png icon24x24.png icon48x48.png application_interface.png 
	@addattrib "LayoutEditor" os::Category Development

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f "LayoutEditor"

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/Applications/sIDE
	@cp "LayoutEditor" $(DIST_DIR)/Applications/sIDE/"LayoutEditor"

install: all
	@echo Installing...
	@mkdir -p /Applications/sIDE
	@cp "LayoutEditor" /Applications/sIDE/"LayoutEditor"

