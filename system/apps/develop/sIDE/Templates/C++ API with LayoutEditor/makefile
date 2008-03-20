# Makefile for App

COPTS	= -Wall -c -O2 -fexceptions

APPBIN	= /Applications/App
OBJS	= main.o application.o mainwindow.o

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

all: objs 'App'

objs:
	@mkdir -p objs

'App': $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o 'App' -lsyllable
	@addattrib 'App' os::Category Other

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f 'App'

deps:

install: all
	@echo Installation...
	@mkdir -p $(IMAGE)/Applications/App
	@cp 'App' $(IMAGE)/Applications/App/'App'

dist: install
