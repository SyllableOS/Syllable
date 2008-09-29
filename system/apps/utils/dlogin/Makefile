# Makefile for dlogin

COPTS = -c -fexceptions -ffast-math

APPBIN = /system/bin
OBJS = main.o application.o mainwindow.o loginview.o loginimageview.o commonfuncs.o shutdown.o keymap.o

CATALOGS = da de el es fr hu nl ru sl sv

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

all : objs catalogs dlogin

objs:
	@mkdir -p objs

dlogin: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o dlogin -lsyllable -lcrypt
	@echo Adding resources...
	@rescopy dlogin -r images/*.png
	@rescopy -a dlogin resources/dlogin.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a dlogin $$CATALOG/dlogin.catalog=resources/$$CATALOG/dlogin.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c dlogin.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t dlogin.cd; \
		cd ..; \
	done;

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f dlogin
	@rm resources/dlogin.catalog resources/dlogin.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/dlogin.catalog; \
	done;

deps:

install: all
	@echo Installing...
	@cp dlogin $(APPBIN)/dlogin

dist: all
	@echo Distribution...
	@cp dlogin $(IMAGE)/$(APPBIN)/dlogin
