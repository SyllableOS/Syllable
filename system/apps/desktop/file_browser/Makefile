# Makefile for FileBrowser

COPTS = -c -fexceptions -ffast-math

APPBIN = /system/bin
OBJS = main.o application.o mainwindow.o toolbar.o

CATALOGS = da de el es fr hu nl ru sv

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

all : objs catalogs FileBrowser

objs:
	@mkdir -p objs

FileBrowser: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o FileBrowser -lsyllable
	@echo Adding resources...
	@rescopy FileBrowser -r home.png up.png breaker.png view.png icon24x24.png icon48x48.png reload.png back.png 
	rescopy -a FileBrowser resources/FileBrowser.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a FileBrowser $$CATALOG/FileBrowser.catalog=resources/$$CATALOG/FileBrowser.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c FileBrowser.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t FileBrowser.cd; \
		cd ..; \
	done;

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f FileBrowser
	@rm resources/FileBrowser.catalog resources/FileBrowser.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/FileBrowser.catalog; \
	done;

deps:

install: all
	@echo Installing...
	@cp FileBrowser $(APPBIN)/FileBrowser

dist: all
	@echo Distribution...
	@cp FileBrowser $(IMAGE)/$(APPBIN)/FileBrowser

