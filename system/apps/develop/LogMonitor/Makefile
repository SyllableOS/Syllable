CXXFLAGS += -I. -c

OBJDIR = objs
OBJS = main tailwin

CATALOGS = ru

EXE = LogMonitor

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: catalogs $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) images/*.png
	rescopy $(EXE) -a resources/$(EXE).catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a sIDE $$CATALOG/$(EXE).catalog=resources/$$CATALOG/$(EXE).catalog; \
	done;
	addattrib $(EXE) os::Category Development

catalogs:
	cd resources; \
	catcomp -c $(EXE).catfish; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t $(EXE).catfish; \
		cd ..; \
	done;

$(OBJDIR):
	mkdir -p $(OBJDIR)


install: all
	mkdir -p $(IMAGE)/applications/sIDE/
	install -s $(EXE) $(IMAGE)/applications/sIDE/$(EXE)

dist: install

clean:
	-rm $(OBJS)
	-rm $(EXE)
	rm resources/$(EXE).catalog resources/$(EXE).h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/$(EXE).catalog; \
	done;

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@
