COPTS   = -I$(TOP)/src -c -Wall

APPBIN = /Applications/sIDE
OBJS = AddWindow.o AddLibrary.o ProjectPrefs.o GroupWindow.o ProjectWindow.o NewProject.o project.o main.o 

CATALOGS = da de el fr hu nl pt-br ru sv

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(COPTS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(COPTS) $< -o $@


all: objs sIDE

objs:
	mkdir objs

sIDE: catalogs $(OBJS)
	$(CXX) $(OBJS) -o sIDE -lsyllable
	strip --strip-all sIDE
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
	mkdir -p $(IMAGE)$(APPBIN)
	cp -f sIDE $(IMAGE)$(APPBIN)/
	mkdir -p $(IMAGE)$(APPBIN)/Templates
	cp -f Templates/*.zip $(IMAGE)$(APPBIN)/Templates/

dist: install

clean:
	rm -f $(OBJDIR)/* sIDE
	rm -f Templates/*.zip
	rm resources/sIDE.catalog resources/sIDE.h; \
	for CATALOG in $(CATALOGS);do \
	rm resources/$$CATALOG/sIDE.catalog; \
	done;

