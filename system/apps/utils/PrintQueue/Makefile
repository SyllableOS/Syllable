CXXFLAGS += -I. -c

OBJDIR = objs
EXE = print-queue

CATALOGS = da de sv

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(EXE)))

all: catalogs $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -lcups -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png
	rescopy $(EXE) -a resources/PrintQueue.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy $(EXE) -a $$CATALOG/PrintQueue.catalog=resources/$$CATALOG/PrintQueue.catalog; \
	done;

$(OBJDIR):
	mkdir -p $(OBJDIR)

catalogs:
	cd resources; \
	catcomp -c PrintQueue.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t PrintQueue.cd; \
		cd ..; \
	done;

deps:

install: all
	install -s $(EXE) /system/bin/$(EXE)

dist: all
	install -s $(EXE) $(IMAGE)/system/bin/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)
	-rm resources/PrintQueue.catalog resources/PrintQueue.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/PrintQueue.catalog; \
	done;

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

