CFLAGS   += -c -fexceptions -ffast-math
CXXFLAGS += -c -fexceptions -ffast-math
AOPTS   = -c
APPBIN = /system/drivers/dock
VPATH = ./
OBJDIR	:= objs
OBJS = ColdFishRemote.o

all :	$(OBJDIR) $(OBJS) $(OBJDIR)/ColdFishRemote.so

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/ColdFishRemote.so: $(OBJS)
	$(CXX) -shared -Xlinker "-soname=ColdFishRemote.so" $(OBJS) -lsyllable -o $(OBJDIR)/ColdFishRemote.so
	rescopy -r $(OBJDIR)/ColdFishRemote.so images/*.png

install:  $(OBJDIR)/ColdFishRemote.so
	mkdir -p $(APPBIN)
	install -s $(OBJDIR)/ColdFishRemote.so $(APPBIN)/ColdFishRemote.so

dist:  $(OBJDIR)/ColdFishRemote.so
	mkdir -p $(DIST_DIR)/atheos/sys/drivers/dock
	install -s $(OBJDIR)/ColdFishRemote.so $(DIST_DIR)/atheos/sys/drivers/dock/ColdFishRemote.so

deps: $(OBJDIR)

clean:
	rm -f $(OBJS) $(OBJDIR)/*.d $(OBJDIR)/ColdFishRemote.so

-include $(OBJDIR)/*.d

