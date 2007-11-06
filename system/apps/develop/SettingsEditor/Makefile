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
#	for CATALOG in $(CATALOGS);do \
#		echo $$CATALOG; \
#		rescopy $(EXE) -a $$CATALOG/SettingsEditor.catalog=resources/$$CATALOG/SettingsEditor.catalog; \
#	done;

$(OBJDIR):
	mkdir -p $(OBJDIR)

#catalogs:
#	cd resources; \
#	catcomp -c SettingsEditor.cd; \
#	for CATALOG in $(CATALOGS);do \
#		echo $$CATALOG; \
#		cd $$CATALOG; \
#		catcomp -t SettingsEditor.cd; \
#		cd ..; \
#	done;

install: all
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

dist: install

clean:
	-rm $(OBJS)
	-rm $(EXE)

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

