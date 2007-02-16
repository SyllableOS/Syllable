CXXFLAGS += -I. -Wall -c

OBJDIR = objs
OBJS = printers print_view config model_window

CATALOGS = da hu ru

EXE = Printers

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: catalogs $(OBJDIR) $(OBJS) 
	g++ $(OBJS) -lsyllable -lcups -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png
	rescopy $(EXE) -a resources/Printers.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy $(EXE) -a $$CATALOG/Printers.catalog=resources/$$CATALOG/Printers.catalog; \
	done;

$(OBJDIR):
	mkdir -p $(OBJDIR)

catalogs:
	cd resources; \
	catcomp -c Printers.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t Printers.cd; \
		cd ..; \
	done;

install: all
	mkdir -p /Applications/Preferences
	install -s $(EXE) /Applications/Preferences/$(EXE)

dist: all
	mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)
	-rm resources/Printers.catalog resources/Printers.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Printers.catalog; \
	done;

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@g++ $(CXXFLAGS) $< -o $@

