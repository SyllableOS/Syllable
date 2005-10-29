CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/extensions/dock
VPATH = ./
OBJDIR	:= objs
OBJS = Address.o Settings.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/Address.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/Address.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=Address.so" $(OBJS) -lsyllable -o $(OBJDIR)/Address.so
	rescopy -r $(OBJDIR)/Address.so resources/*.png resources/*.html

install:  $(OBJDIR)/Address.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/Address.so $(APPBIN)/Address.so

dist:  $(OBJDIR)/Address.so
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dock
	install -s $(OBJDIR)/Address.so $(DIST_DIR)/atheos/sys/drivers/dock/Address.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/Address.so

-include $(OBJDIR)/*.d


