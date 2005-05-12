CFLAGS   += -c -fexceptions
CXXFLAGS += -c -fexceptions
AOPTS    = -c

APPBIN = /system

VPATH = ./

OBJS = registrar.o

OBJDIR	:= objs

include $(ATHEOS_SRC)/makefile.rules

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/registrar

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/registrar: $(OBJS)
	$(CXX) $(OBJS) -lsyllable -o registrar
#	rescopy mediaserver -r *.png

deps:

install:  $(OBJDIR)/registrar
	
	install -s registrar $(APPBIN)/registrar

dist:  $(OBJDIR)/registrar
	install -s registrar $(DIST_DIR)/atheos/sys/registrar

deps: $(OBJDIR) $(DEPS)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d registrar

-include $(OBJDIR)/*.d

