COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -O2 -c -Wall

OBJS =	vmware.o vmware_cursor.o vmware_fb.o vmware_fifo.o vmware_io.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

DRIVER := $(OBJDIR)/vmware

all :	objs $(OBJDIR)/vmware

objs:
	mkdir objs

$(OBJDIR)/vmware: $(OBJS)
	$(CXX) -plugin $(OBJS) -o $(OBJDIR)/vmware -lsyllable

$(OBJDIR)/vmware.o: vmware.cpp vmware.h
	$(CC) $(COPTS) vmware.cpp -o $@

$(OBJDIR)/vmware_cursor.o: vmware_cursor.cpp vmware.h
	$(CC) $(COPTS) vmware_cursor.cpp -o $@

$(OBJDIR)/vmware_fb.o: vmware_fb.cpp vmware.h
	$(CC) $(COPTS) vmware_fb.cpp -o $@

$(OBJDIR)/vmware_fifo.o: vmware_fifo.cpp vmware.h
	$(CC) $(COPTS) vmware_fifo.cpp -o $@

$(OBJDIR)/vmware_io.o: vmware_io.cpp vmware.h
	$(CC) $(COPTS) vmware_io.cpp -o $@

deps:

install: $(DRIVER)
	cp -f $(DRIVER) /system/drivers/appserver/video/

dist: $(DRIVER)
	cp -f $(DRIVER) $(DIST_DIR)/atheos/sys/drivers/appserver/video/

clean:
	-rm $(OBJS) $(OBJDIR)/vmware

install:
	cp -f $(OBJDIR)/vmware /system/drivers/appserver/video/

