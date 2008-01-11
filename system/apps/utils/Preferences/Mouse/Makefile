#Makefile for Mouse
COPTS = -c -fexceptions -Wall -o2
LOPTS = -lsyllable

CATALOGS = sv

OBJS = 	main.o appwindow.o

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CXX) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@

all : objs mouse

objs:
	@echo Creating subdirectories...
	@mkdir -p objs

mouse: $(OBJS)
	@echo Linking...
	@$(CXX) $(LOPTS)  $(OBJS) -o mouse
	@echo Adding resources...
	@rescopy -r mouse resources/mouse.catalog
	@rescopy -a mouse images/*
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a mouse $$CATALOG/mouse.catalog=resources/$$CATALOG/mouse.catalog; \
	done;

clean:
	@echo Cleaning...
	@rm -r $(OBJDIR)
	@rm -f mouse
