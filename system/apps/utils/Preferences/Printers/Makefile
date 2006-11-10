CXXFLAGS += -I. -Wall -c

OBJDIR = objs
OBJS = printers print_view config model_window

EXE = Printers

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: $(OBJDIR) $(OBJS) 
	g++ $(OBJS) -lsyllable -lcups -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) icons/*.png

$(OBJDIR):
	mkdir -p $(OBJDIR)

install: all
	mkdir -p /Applications/Preferences
	install -s $(EXE) /Applications/Preferences/$(EXE)

dist: all
	mkdir -p $(DIST_DIR)/atheos/Applications/Preferences
	install -s $(EXE) $(DIST_DIR)/atheos/Applications/Preferences/$(EXE)

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@g++ $(CXXFLAGS) $< -o $@

