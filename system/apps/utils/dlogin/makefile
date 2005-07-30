#Makefile for dlogin
COPTS = -Wall -c -O2 -pipe -fexceptions -march=i586 -ffast-math -fomit-frame-pointer

APPBIN = /bin/dlogin
OBJS = main.o application.o mainwindow.o loginview.o loginimageview.o commonfuncs.o shutdown.o

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CXX) $(COPTS) -x assembler-with-cpp $< -o $@

all : objs dlogin

objs:
	@mkdir -p objs

dlogin: $(OBJS)
	@echo Linking...
	@g++ $(OBJS) -o dlogin  -lsyllable -lstdc++ -lcrypt
	@echo Adding resources...
	@rescopy dlogin -r ./images/* 

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f dlogin

deps:

dist: all
	@echo Distribution...
	@cp dlogin $(DIST_DIR)/bin/dlogin

install: all
	@echo Installing...
	@cp dlogin /bin/dlogin

