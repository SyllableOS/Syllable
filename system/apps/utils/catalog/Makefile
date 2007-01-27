CFLAGS	+= -c
CXXFLAGS += -c
OBJDIR = objs

EXES	= $(OBJDIR)/catalogtool $(OBJDIR)/catcomp

catalogtool_OBJS = catalogtool.o ccatalog.o
catcomp_OBJS = catcomp.o ccatalog.o

catalogtool_OBJS	:= $(addprefix $(OBJDIR)/,$(catalogtool_OBJS))
catcomp_OBJS	:= $(addprefix $(OBJDIR)/,$(catcomp_OBJS))

include $(ATHEOS_SRC)/makefile.rules

all:  $(OBJDIR) $(EXES)

$(OBJDIR):
	-mkdir $(OBJDIR)

$(OBJDIR)/catcomp: $(catcomp_OBJS)
	$(CXX) $(catcomp_OBJS) -lsyllable -o $@
	rescopy -r $(OBJDIR)/catcomp catcomp.catalog=resources/catcomp.catalog
	rescopy -a $(OBJDIR)/catcomp da/catcomp.catalog=resources/da/catcomp.catalog
	rescopy -a $(OBJDIR)/catcomp nl/catcomp.catalog=resources/nl/catcomp.catalog
	rescopy -a $(OBJDIR)/catcomp sv/catcomp.catalog=resources/sv/catcomp.catalog

$(OBJDIR)/catalogtool: $(catalogtool_OBJS)
	$(CXX) $(catalogtool_OBJS) -lsyllable -o $@

deps:

install:
	cp -f $(EXES) /system/bin/

dist:
	cp -f $(EXES) $(IMAGE)/system/bin/

clean:
	-rm -Rv $(OBJDIR)

