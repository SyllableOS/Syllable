CC = g++
COPTS   = -I$(TOP)/src  -c -Wall

APPBIN = /Applications/sIDE
OBJS = AddWindow.o AddLibrary.o ProjectPrefs.o GroupWindow.o ProjectWindow.o NewProject.o project.o main.o 

CATALOGS = de

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CC) $(COPTS) $< -o $@


all :	objs sIDE

objs:
	mkdir objs

sIDE: catalogs $(OBJS)
	g++ $(OBJS) -o sIDE -lsyllable
	rescopy sIDE -r ./images/*.png
	rescopy -a sIDE resources/sIDE.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a sIDE $$CATALOG/sIDE.catalog=resources/$$CATALOG/sIDE.catalog; \
	done;
	addattrib sIDE os::Category Development
	build_templates.sh

catalogs:
	cd resources; \
	catcomp -c sIDE.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t sIDE.cd; \
		cd ..; \
	done;

install: sIDE
	mkdir -p $(APPBIN)
	cp -f sIDE $(APPBIN)/sIDE
	mkdir -p $(APPBIN)/Templates
	cp -f Templates/*.zip $(APPBIN)/Templates/

dist: sIDE
	mkdir -p $(DIST_DIR)/Applications/sIDE
	cp -f sIDE $(DIST_DIR)/Applications/sIDE/
	mkdir -p $(DIST_DIR)/Applications/sIDE/Templates
	cp -f Templates/*.zip $(DIST_DIR)/Applications/sIDE/Templates/

clean:
	rm -f $(OBJDIR)/* sIDE
	rm -f Templates/*.zip
	rm resources/sIDE.catalog resources/sIDE.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/sIDE.catalog; \
	done;

