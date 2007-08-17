CXXFLAGS += -I. -c

OBJDIR = objs
OBJS = printers print_view config model_window

CATALOGS = bg da de el es fr hu nl ru sv

EXE = Printers

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: catalogs $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -lcups -o $(EXE)
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
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

dist: install

clean:
	-rm $(OBJS)
	-rm $(EXE)
	-rm resources/Printers.catalog resources/Printers.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Printers.catalog; \
	done;

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@
