#Makefile for dlogin
COPTS = -c -fexceptions -ffast-math

APPBIN = /bin/dlogin
OBJS = main.o application.o mainwindow.o loginview.o loginimageview.o commonfuncs.o shutdown.o

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

all : objs dlogin

objs:
	@mkdir -p objs

dlogin: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o dlogin -lsyllable -lcrypt
	@echo Adding resources...
	@rescopy dlogin -r images/*.png

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f dlogin

deps:

dist: all
	@echo Distribution...
	@cp dlogin $(DIST_DIR)/atheos/sys/bin/dlogin

install: all
	@echo Installing...
	@cp dlogin /bin/dlogin
