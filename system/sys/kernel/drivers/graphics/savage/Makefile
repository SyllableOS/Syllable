CFLAGS   = -kernel -fno-PIC -c -I.
AFLAGS   = -c


OBJS := savage.o


OBJDIR	:= objs
OBJS	:= $(addprefix $(OBJDIR)/,$(OBJS))


#
#	rules
#

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

$(OBJDIR)/%.o : %.s
	$(CC) $(AFLAGS) -x assembler-with-cpp $< -o $@

$(OBJDIR)/%.o : %.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@




all :	objs $(OBJDIR)/savage

objs:
	mkdir objs

$(OBJDIR)/savage: $(OBJS) Makefile
	$(CC) -kernel $(OBJS) -o $(OBJDIR)/savage

install: all
	cp -f $(OBJDIR)/savage $(IMAGE)/system/drivers/dev/graphics/

dist: install

clean:
	-rm -f $(OBJS) $(OBJDIR)/savage

