CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c

CATALOGS = da de es fr hu nl pl pt-br ru sv

APPBIN = /Applications/Media

VPATH = ./

OBJS = lcd.o SelectWin.o ColdFish.o cimagebutton.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) catalogs $(OBJS) $(OBJDIR)/ColdFish

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFish: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -lmedia -o ColdFish
	rescopy -r ColdFish images/*.png
	rescopy -a ColdFish resources/coldfish.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a ColdFish $$CATALOG/coldfish.catalog=resources/$$CATALOG/coldfish.catalog; \
	done;
	addattrib ColdFish os::Category Media

catalogs:
	cd resources; \
	catcomp -c coldfish.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t coldfish.cd; \
		cd ..; \
	done;

deps:

install:  $(OBJDIR)/ColdFish
	mkdir -p $(IMAGE)/$(APPBIN)
	cp ColdFish $(IMAGE)/$(APPBIN)

dist: install

deps: $(OBJDIR) $(DEPS)

clean: catalog-clean
	rm -f $(OBJS) $(OBJDIR)/*.d ColdFish

catalog-clean:
	-rm resources/coldfish.catalog resources/coldfish.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/coldfish.catalog; \
	done;

-include $(OBJDIR)/*.d

