CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = ColdFishRemote.o

CATALOGS = da de el es fr hu nl pl pt-br ru sv

all :	$(OBJDIR) catalogs $(OBJS) $(OBJDIR)/ColdFishRemote.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFishRemote.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=ColdFishRemote.so" $(OBJS) -lsyllable -o $(OBJDIR)/ColdFishRemote.so
	rescopy -r $(OBJDIR)/ColdFishRemote.so images/*.png
	rescopy -a $(OBJDIR)/ColdFishRemote.so resources/ColdFishRemote.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(OBJDIR)/ColdFishRemote.so $$CATALOG/ColdFishRemote.catalog=resources/$$CATALOG/ColdFishRemote.catalog; \
	done;


catalogs:
	cd resources; \
	catcomp -c ColdFishRemote.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t ColdFishRemote.cd; \
		cd ..; \
	done;

install:  $(OBJDIR)/ColdFishRemote.so
	install -s $(OBJDIR)/ColdFishRemote.so $(APPBIN)/ColdFishRemote.so

dist:  $(OBJDIR)/ColdFishRemote.so
	install -s $(OBJDIR)/ColdFishRemote.so $(IMAGE)/$(APPBIN)/ColdFishRemote.so

deps: $(OBJDIR)

clean: catalog-clean
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/ColdFishRemote.so

catalog-clean:
	-rm resources/ColdFishRemote.catalog resources/ColdFishRemote.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/ColdFishRemote.catalog; \
	done;


-include $(OBJDIR)/*.d

