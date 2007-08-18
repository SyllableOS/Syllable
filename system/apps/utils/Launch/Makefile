# Makefile for Launch

COPTS = -c -fexceptions -ffast-math

APPBIN = /Applications/
OBJS = main.o application.o commonfuncs.o execute.o mainwindow.o

EXE = Launch

CATALOGS = da de el es hu nl pl pt-br ru sv

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

all : objs $(EXE)

objs:
	@mkdir -p objs

$(EXE): catalogs $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -o $(EXE) -lsyllable
	@echo Adding resources...
	@rescopy $(EXE) -r images/*.png
	rescopy -a $(EXE) resources/$(EXE).catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c $(EXE).cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t $(EXE).cd; \
		cd ..; \
	done;

clean: catalog-clean
	@echo Cleaning...
	-rm $(OBJS)
	-rm $(EXE)

catalog-clean:
	-rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

deps:

install: all
	@echo Installing...
	@cp $(EXE) $(IMAGE)/Applications/

dist: install

