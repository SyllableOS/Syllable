# Makefile for desktop

COPTS = -c -fexceptions -ffast-math

CATALOGS = da de el es fr hu nl ru sl sv

APPBIN = /system/bin
OBJS = main.o application.o desktop.o

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

all : objs catalogs desktop

objs:
	@mkdir -p objs

desktop: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o desktop -lsyllable
	@echo Adding resources...
	@rescopy desktop -r icon48x48.png
	@rescopy -a desktop resources/desktop.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a desktop $$CATALOG/desktop.catalog=resources/$$CATALOG/desktop.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c desktop.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t desktop.cd; \
		cd ..; \
	done;

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f desktop
	@rm resources/desktop.catalog resources/desktop.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/desktop.catalog; \
	done;

deps:

dist: all
	@echo Distribution...
	@cp desktop $(IMAGE)/system/bin/desktop

install: all
	@echo Installing...
	@cp desktop /system/bin/desktop
