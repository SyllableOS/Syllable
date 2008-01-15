#Makefile for Mouse
CXXFLAGS += -c -fexceptions
LIBS = -lsyllable

CATALOGS = sv

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

mouse: $(OBJS)
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

clean:
	@echo Cleaning...
	@rm -r $(OBJDIR)
	@rm -f $(EXE)

install: $(EXE)
	install -s $(EXE) $(IMAGE)/Applications/Preferences/$(EXE)

dist: install

