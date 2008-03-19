CXXFLAGS += -I. -c

OBJDIR = objs
OBJS = main tailwin

CATALOGS = 

EXE = LogMonitor

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) images/*.png
	rescopy $(EXE) -a resources/LogMonitor.catalog
	addattrib $(EXE) os::Category Development

$(OBJDIR):
	mkdir -p $(OBJDIR)


install: all
	mkdir -p $(IMAGE)/Applications/sIDE/
	install -s $(EXE) $(IMAGE)/Applications/sIDE/$(EXE)

dist: install

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

