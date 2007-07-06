#Makefile for Contact
CXXFLAGS += -Wall -c -fexceptions

EXE = Contact

CATALOGS = bg da de es fr hu nl ru sv

OBJDIR = objs
OBJS = main.o application.o mainwindow.o
OBJS := $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(CXXFLAGS) $< -o $@

all : catalogs $(EXE)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(EXE): $(OBJDIR) $(OBJS)
	@echo Linking...
	@$(CXX) $(OBJS) -lsyllable -o $(EXE)
	@echo Adding resources...
	@rescopy $(EXE) -r images/*.png
	@rescopy $(EXE) -a resources/Contact.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy $(EXE) -a $$CATALOG/Contact.catalog=resources/$$CATALOG/Contact.catalog; \
	done;
	@addattrib $(EXE) os::Category Office

catalogs:
	cd resources; \
	catcomp -c Contact.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t Contact.cd; \
		cd ..; \
	done;

clean:
	@echo Cleaning...
	@rm -f $(OBJDIR)/*
	@rm -f $(EXE)
	@rm resources/Contact.catalog resources/Contact.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Contact.catalog; \
	done;

deps:

dist: install

install: all
	@echo Installing...
	@mkdir -p $(IMAGE)/Applications/Contact
	@cp $(EXE) $(IMAGE)/Applications/Contact/$(EXE)

