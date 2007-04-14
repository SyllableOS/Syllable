#Makefile for Contact
COPTS = -Wall -c -fexceptions

APPBIN = /Applications/Contact
OBJS = main.o application.o mainwindow.o

CATALOGS = da

OBJDIR := objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

# Rules
$(OBJDIR)/%.o : %.c
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	@echo Compiling : $<
	@$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.s
	@echo Assembling : $<
	@$(CC) $(COPTS) -x assembler-with-cpp $< -o $@

all : catalogs objs "Contact"

$(OBJDIR)/mainwindow.o: mainwindowLayout.cpp

objs:
	@mkdir -p objs

"Contact": $(OBJS)
	@echo Linking...
	@gcc $(OBJS) -o "Contact"  -lstdc++ -lsyllable
	@echo Adding resources...
	@rescopy "Contact" -r images/*.png
	@rescopy "Contact" -a resources/Contact.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy "Contact" -a $$CATALOG/Contact.catalog=resources/$$CATALOG/Contact.catalog; \
	done;
	@addattrib "Contact" os::Category Office

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
	@rm -f "Contact"
	@rm resources/Contact.catalog resources/Contact.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/Contact.catalog; \
	done;

deps:

dist: all
	@echo Distribution...
	@mkdir -p $(DIST_DIR)/Applications/Contact
	@cp "Contact" $(DIST_DIR)/Applications/Contact/"Contact"

install: all
	@echo Installing...
	@mkdir -p /Applications/Contact
	@cp "Contact" /Applications/Contact/"Contact"

