COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -c

OBJS =	vmware.o vmware_cursor.o vmware_fb.o vmware_fifo.o vmware_io.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

DRIVER := $(OBJDIR)/vmware

all :	objs $(OBJDIR)/vmware

objs:
	mkdir objs

$(OBJDIR)/vmware: $(OBJS)
	$(CXX) -plugin -Xlinker -Bsymbolic -nodefaultlibs $(OBJS) -o $(OBJDIR)/vmware

$(OBJDIR)/vmware.o: vmware.cpp vmware.h
	$(CXX) $(COPTS) vmware.cpp -o $@

$(OBJDIR)/vmware_cursor.o: vmware_cursor.cpp vmware.h
	$(CXX) $(COPTS) vmware_cursor.cpp -o $@

$(OBJDIR)/vmware_fb.o: vmware_fb.cpp vmware.h
	$(CXX) $(COPTS) vmware_fb.cpp -o $@

$(OBJDIR)/vmware_fifo.o: vmware_fifo.cpp vmware.h
	$(CXX) $(COPTS) vmware_fifo.cpp -o $@

$(OBJDIR)/vmware_io.o: vmware_io.cpp vmware.h
	$(CXX) $(COPTS) vmware_io.cpp -o $@

deps:

install: $(DRIVER)
	cp -f $(DRIVER) $(IMAGE)/system/drivers/appserver/video/

dist: install

clean:
	-rm $(OBJS) $(OBJDIR)/vmware

