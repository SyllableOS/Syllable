CXXFLAGS += -I. -c

OBJDIR = objs
EXE = print-queue

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(EXE)))

all: $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -lcups -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png

$(OBJDIR):
	mkdir -p $(OBJDIR)

deps:

install: all
	install -s $(EXE) /system/bin/$(EXE)

dist: all
	install -s $(EXE) $(IMAGE)/system/bin/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

