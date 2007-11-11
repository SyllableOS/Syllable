CXXFLAGS += -I. -c

OBJDIR = objs
OBJS = app editstring editwin mainwin msgtotext strreq

CATALOGS = bg da de es fr hu nl ru sv

EXE = SettingsEditor

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) images/*.png
	rescopy $(EXE) -a resources/SettingsEditor.catalog


$(OBJDIR):
	mkdir -p $(OBJDIR)


install: all
	mkdir $(IMAGE)/Applications/SettingsEditor/
	install -s $(EXE) $(IMAGE)/Applications/SettingsEditor/$(EXE)

dist: install

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

