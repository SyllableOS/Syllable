CFLAGS   += -kernel -fno-PIC -c -I. -D__ENABLE_DEBUG__
AFLAGS   = -c

C_INCLUDE_PATH=$(ATHEOS_INCLUDE_PATH)

OBJS := via.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))

GSRCDIR := ../generic
GOBJDIR := $(GSRCDIR)/objs

#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(GOBJDIR)/generic.o $(OBJDIR)/agp_via

objs:
	mkdir objs
	mkdir $(GOBJDIR)

$(GOBJDIR)/generic.o: $(GSRCDIR)/generic.c
	$(CC) $(CFLAGS) $(GSRCDIR)/generic.c -o $(GOBJDIR)/generic.o

$(OBJDIR)/agp_via: $(OBJS) $(GOBJDIR)/generic.o Makefile
	$(CC) -kernel $(OBJS) $(GOBJDIR)/generic.o -o $(OBJDIR)/agp_via

install: all
	cp -f $(OBJDIR)/agp_via $(IMAGE)/system/drivers/dev/hcd/agp_via

dist: install

clean:
	-rm -f $(OBJDIR)/*
	-rm -f $(GOBJDIR)/*

