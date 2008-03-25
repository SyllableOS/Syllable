# Makefile for LayoutEditor

COPTS = -Wall -c -fexceptions -O2

APPBIN = /Applications/sIDE
OBJS = AppWindow.o EditList.o EditString.o PropertyRow.o WidgetsControls.o WidgetsImage.o WidgetsList.o WidgetsMenu.o WidgetsNodes.o WidgetsUtils.o application.o main.o mainwindow.o ccatalog.o

OBJDIR	:= objs
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

all : translations objs "LayoutEditor"


translations:

objs:
	@mkdir -p objs

"LayoutEditor": $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o "LayoutEditor" -lsyllable
	@echo Adding resources...
	@rescopy "LayoutEditor" -r images/breaker.png images/open.png images/save.png icon24x24.png icon48x48.png application_interface.png 
	strip --strip-all LayoutEditor
	@addattrib "LayoutEditor" os::Category Development

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f "LayoutEditor"

deps:

install: all
	@echo Installing...
	@mkdir -p $(IMAGE)/Applications/sIDE
	@cp "LayoutEditor" $(IMAGE)/Applications/sIDE/"LayoutEditor"

dist: install

