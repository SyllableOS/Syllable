COPTS   = -I$(TOP)/src $(shell freetype-config --cflags) -O2 -c -Wall

OBJS =	vmware.o vmware_cursor.o vmware_fb.o vmware_fifo.o vmware_io.o

OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

all :	objs $(OBJDIR)/vmware

objs:
	mkdir objs

$(OBJDIR)/vmware: $(OBJS)
	gcc -plugin $(OBJS) -o $(OBJDIR)/vmware -lsyllable -lstdc++

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

clean:
	-rm $(OBJS) $(OBJDIR)/vmware
