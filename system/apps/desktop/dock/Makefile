# Makefile for Dock

COPTS = -c -fexceptions -ffast-math

APPBIN = /system/bin
OBJS = Dock.o DockMenu.o

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

CATALOGS = da de el es fr hu nl ru sl sv

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

all : objs catalogs Dock

objs:
	@mkdir -p objs

Dock: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o Dock -lsyllable
	@echo Adding resources...
	@rescopy Dock -r logout.png Syllable.png window.png folder.png
	rescopy -a Dock resources/Dock.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a Dock $$CATALOG/Dock.catalog=resources/$$CATALOG/Dock.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c Dock.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t Dock.cd; \
		cd ..; \
	done;

clean: catalog-clean
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f Dock

catalog-clean:
	-rm resources/Dock.catalog resources/Dock.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Dock.catalog; \
	done;

deps:

install: all
	@echo Installing...
	@cp Dock $(IMAGE)/system/bin/Dock

dist: install
