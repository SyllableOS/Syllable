#Makefile for dlogin
COPTS = -c -fexceptions -ffast-math

APPBIN = /bin/dlogin
OBJS = main.o application.o mainwindow.o loginview.o loginimageview.o commonfuncs.o shutdown.o

CATALOGS = da nl es hu sv

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

dist: all
	@echo Distribution...
	@cp dlogin $(DIST_DIR)/atheos/sys/bin/dlogin

install: all
	@echo Installing...
	@cp dlogin /bin/dlogin
