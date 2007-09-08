CFLAGS	+= -c
CXXFLAGS += -c
OBJDIR = objs

CATALOGS = da de el fr hu nl pt-br ru sv

EXES	= $(OBJDIR)/catalogtool $(OBJDIR)/catcomp

catalogtool_OBJS = catalogtool.o ccatalog.o
catcomp_OBJS = catcomp.o ccatalog.o

catalogtool_OBJS	:= $(addprefix $(OBJDIR)/,$(catalogtool_OBJS))
catcomp_OBJS	:= $(addprefix $(OBJDIR)/,$(catcomp_OBJS))

include $(ATHEOS_SRC)/makefile.rules

all: catalogs $(OBJDIR) $(EXES)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/catcomp: $(catcomp_OBJS)
	$(CXX) $(catcomp_OBJS) -lsyllable -o $@
	rescopy -r $(OBJDIR)/catcomp resources/catcomp.catalog
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		rescopy -a $(OBJDIR)/catcomp $$CATALOG/catcomp.catalog=resources/$$CATALOG/catcomp.catalog; \
	done;

$(OBJDIR)/catalogtool: $(catalogtool_OBJS)
	$(CXX) $(catalogtool_OBJS) -lsyllable -o $@

catalogs:
	cd resources; \
	catcomp -c catcomp.cd; \
	for CATALOG in $(CATALOGS);do \
		echo $$CATALOG; \
		cd $$CATALOG; \
		catcomp -t catcomp.cd; \
		cd ..; \
	done;

deps:

install:
	cp -f $(EXES) /system/bin/

dist:
	cp -f $(EXES) $(IMAGE)/system/bin/

clean:
	-rm -Rv $(OBJDIR)
	rm resources/catcomp.catalog resources/catcomp.h; \
	for CATALOG in $(CATALOGS);do \
		rm resources/$$CATALOG/catcomp.catalog; \
	done;

