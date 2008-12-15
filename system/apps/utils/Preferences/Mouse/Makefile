# Makefile for Mouse Preference

CXXFLAGS += -c -fexceptions
LIBS = -lsyllable

CATALOGS = ru sv

OBJS = 	main.o appwindow.o

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

EXE = Mouse

# Rules
$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

all : objs mouse

objs:
	@echo Creating subdirectories...
	@mkdir -p objs

mouse: catalogs $(OBJS)
	@echo Linking...
	@$(CXX) $(LFLAGS) $(LIBS) $(OBJS) -o $(EXE)
	@strip --strip-all $(EXE)
	@echo Adding resources...
	@rescopy -r $(EXE) resources/mouse.catalog
	@rescopy -a $(EXE) images/icon*.png
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(EXE) $$CATALOG/mouse.catalog=resources/$$CATALOG/mouse.catalog; \
	done;

catalogs:
	cd resources; \
	catcomp -c mouse.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t mouse.cd; \
		cd ..; \
	done;

clean: catalog-clean
	@echo Cleaning...
	@rm -r $(OBJDIR)
	@rm -f $(EXE)

catalog-clean:
	-rm resources/mouse.catalog resources/mouse.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/mouse.catalog; \
	done;


install: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

dist: install
