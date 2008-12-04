CXXFLAGS += -I. -c

OBJDIR = objs
OBJS = app editstring editwin mainwin msgtotext strreq

CATALOGS = ru

EXE = SettingsEditor

OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(OBJS)))

all: catalogs $(OBJDIR) $(OBJS) 
	$(CXX) $(OBJS) -lsyllable -o $(EXE)
	strip --strip-all $(EXE)
	rescopy -r $(EXE) images/*.png
	rescopy $(EXE) -a resources/SettingsEditor.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a sIDE $$CATALOG/SettingsEditor.catalog=resources/$$CATALOG/SettingsEditor.catalog; \
	done;


$(OBJDIR):
	mkdir -p $(OBJDIR)

catalogs:
	cd resources; \
	catcomp -c SettingsEditor.catfish; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t SettingsEditor.catfish; \
		cd ..; \
	done;

install: all
	mkdir $(IMAGE)/Applications/SettingsEditor/
	install -s $(EXE) $(IMAGE)/Applications/SettingsEditor/$(EXE)

dist: install

clean:
	-rm $(OBJS)
	-rm $(EXE)
	rm resources/SettingsEditor.catalog resources/SettingsEditor.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/SettingsEditor.catalog; \
	done;

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@
