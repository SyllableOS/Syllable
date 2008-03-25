# Makefile for Sourcery

COPTS = -O2 -Wall -c 

APPBIN = /Applications/sIDE
OBJS = main.o file.o settings.o splash.o application.o editor.o mainwindow.o aboutbox.o statusbar.o formatset.o commonfuncs.o link.o convertdialog.o fileproperties.o finddialog.o replacedialog.o userinserts.o gotodialog.o browser.o settingsdialog.o execute.o toolsdialog.o colorbutton.o

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

all : objs Sourcery

objs:
	@mkdir -p objs

Sourcery: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o Sourcery -lsyllable -lm codeview.a
	@strip --strip-all Sourcery
	@echo Adding resources...
	@rescopy Sourcery -r ./images/*.png ./images/mimetypes/*.png 
	@addattrib Sourcery os::Category Development

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Sourcery

deps:

install: all
	@echo Installing...
	@mkdir -p $(IMAGE)/Applications/sIDE
	@cp Sourcery $(IMAGE)/Applications/sIDE/Sourcery

dist: install

