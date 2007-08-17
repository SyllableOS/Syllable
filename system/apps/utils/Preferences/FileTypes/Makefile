#Makefile for FileTypes
COPTS = -c

CATALOGS = da de el es fr hu nl pt-br ru sv

APPBIN = /Applications/Preferences/
OBJS = main.o application.o mainwindow.o newtype.o

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

all : objs catalogs FileTypes

objs:
	@mkdir -p objs

FileTypes: $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o FileTypes -lsyllable
	@echo Adding resources...
	@rescopy FileTypes -r icon48x48.png
	rescopy -a FileTypes resources/FileTypes.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a FileTypes $$CATALOG/FileTypes.catalog=resources/$$CATALOG/FileTypes.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c FileTypes.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t FileTypes.cd; \
		cd ..; \
	done;

clean: catalog-clean
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f FileTypes

catalog-clean:
	-rm resources/FileTypes.catalog resources/FileTypes.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/FileTypes.catalog; \
	done;

deps:

install: all
	@echo Installing...
	@cp FileTypes $(IMAGE)/$(APPBIN)

dist: install

