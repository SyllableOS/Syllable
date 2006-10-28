#Makefile for Desktop
COPTS = -c

CATALOGS = da es hu pt-br sv

APPBIN = /atheos/Applications/Preferences
OBJS = Prefs.o

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

all : objs catalogs Desktop

objs:
	@mkdir -p objs

Desktop: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o Desktop -lsyllable
	@echo Adding resources...
	@rescopy Desktop -r icon48x48.png icon24x24.png 
	rescopy -a Desktop resources/Desktop.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a Desktop $$CATALOG/Desktop.catalog=resources/$$CATALOG/Desktop.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c Desktop.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t Desktop.cd; \
		cd ..; \
	done;

clean: catalog-clean
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Desktop

catalog-clean:
	-rm resources/Desktop.catalog resources/Desktop.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Desktop.catalog; \
	done;

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	@cp Desktop $(DIST_DIR)/atheos/Applications/Preferences/Desktop

install: all
	@echo Installing...
	@mkdir -p /atheos/Applications/Preferences
	@cp Desktop /atheos/Applications/Preferences/Desktop

